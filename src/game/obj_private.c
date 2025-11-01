#include "game/obj_private.h"

#include <stdio.h>

#include "game/map.h"
#include "game/obj_file.h"
#include "game/object.h"
#include "game/sector.h"

// Represents metadata for one object field bitfield segment.
//
// Each entry corresponds to a logical bitfield belonging to an object,
// stored within the global FieldBitData array. The struct defines where
// the bitfield begins and how many 32-bit words it spans.
typedef struct ObjFieldMeta {
    int word_count; // Number of 32-bit words used by this metadata entry.
    int word_offset; // Starting index of this entry's bitfield in FieldBitData.
} ObjFieldMeta;

typedef struct BitMaskPair {
    uint16_t lower_mask; /**< Mask for lower 16 bits of the input value. */
    uint16_t upper_mask; /**< Mask for upper 16 bits of the input value. */
} BitMaskPair;

// Ensures that the buffer has enough space for 'size' additional bytes,
// reallocating and expanding its capacity if necessary.
static void ensure_memory_capacity(MemoryWriteBuffer* a1, int size);
static void field_metadata_grow_word_array(int a1, int a2);
static void field_metadata_shrink_word_array(int a1, int a2);
static void AdjustFieldOffsets(int start, int end, int inc);
static int convert_bit_index_to_word_index(int a1);
static int convert_bit_index_to_mask(int a1);
static void InitPopCountLookup();
static void ObjBitMaskTable_Init();

// 0x6036A8
static bool g_ObjPrivateEnabled;

// 0x6036FC
static uint8_t* Popcount16Lookup;

// 0x603700
static int FieldMetaCapacity;

// 0x603704
static int FreeIndexCapacity;

// 0x603708
static int FieldBitDataSize;

// Capacity: 0x603704
// Size: 0x603714
//
// 0x60370C
static int* FreeFieldMetaIndices;

// Capacity: 0x603700
// Size: 0x603724
//
// 0x603710
static ObjFieldMeta* FieldMetaTable;

// 0x603714
static int FreeIndexCount;

// Capacity: 0x60371C
//
// 0x603718
static int* FieldBitData;

// 0x60371C
static int FieldBitDataCapacity;

// 0x603720
static BitMaskPair* BitMaskTable;

// 0x603724
static int FieldMetaCount;

// 0x603728
static bool ObjPrivateStorageAllocated;

// 0x4E3F80
void ObjPrivate_Enable()
{
    g_ObjPrivateEnabled = true;
}

// 0x4E3F90
void ObjPrivate_Disable()
{
    g_ObjPrivateEnabled = false;
}

// 0x4E3FA0
void object_field_deallocate(ObjSa* a1)
{
    switch (a1->type) {
    case SA_TYPE_INT32:
        *(int*)a1->ptr = 0;
        break;
    case SA_TYPE_INT64:
    case SA_TYPE_STRING:
    case SA_TYPE_HANDLE:
        if (*(void**)a1->ptr != NULL) {
            FREE(*(void**)a1->ptr);
            *(void**)a1->ptr = NULL;
        }
        break;
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_deallocate((SizeableArray**)a1->ptr);
        }
        break;
    case SA_TYPE_PTR:
        *(intptr_t*)a1->ptr = 0;
        break;
    case SA_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_deallocate((SizeableArray**)a1->ptr);
        }
        break;
    }
}

// Apply the value from a1->storage into the live field at a1->ptr.
// - Allocates/frees for pointer-backed types as needed.
// - For arrays, ensures the SizeableArray exists, then writes element at a1->idx.
// - For HANDLE and STRING, deep-copies the stored value into owned memory.
// - For INT64 and HANDLE, a "null"/zero in storage frees and clears the target.
// 0x4E4000
void object_field_apply_from_storage(ObjSa* obj_field)
{
    switch (obj_field->type) {
    case SA_TYPE_INT32:
        // Plain in-place store.
        *(int*)obj_field->ptr = obj_field->storage.value;
        break;

    case SA_TYPE_INT64:
        // Pointer-backed 64-bit integer.
        // storage.value64 == 0  → free and NULL the target
        // otherwise            → ensure allocation and write the value
        if (obj_field->storage.value64 != 0) {
            if (*(int64_t**)obj_field->ptr == NULL) {
                *(int64_t**)obj_field->ptr = MALLOC(sizeof(int64_t));
            }
            **(int64_t**)obj_field->ptr = obj_field->storage.value64;
        } else {
            if (*(int64_t**)obj_field->ptr != NULL) {
                FREE(*(int64_t**)obj_field->ptr);
                *(int64_t**)obj_field->ptr = NULL;
            }
        }
        break;

    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
        // Ensure array exists with element size = int, then set element at idx.
        if (*(SizeableArray**)obj_field->ptr == NULL) {
            sa_allocate((SizeableArray**)obj_field->ptr, sizeof(int));
        }
        sa_set((SizeableArray**)obj_field->ptr, obj_field->idx, &(obj_field->storage));
        break;

    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
        // Ensure array exists with element size = int64_t, then set element at idx.
        if (*(SizeableArray**)obj_field->ptr == NULL) {
            sa_allocate((SizeableArray**)obj_field->ptr, sizeof(int64_t));
        }
        sa_set((SizeableArray**)obj_field->ptr, obj_field->idx, &(obj_field->storage));
        break;

    case SA_TYPE_SCRIPT:
        // Ensure array exists with element size = Script, then set element at idx.
        if (*(SizeableArray**)obj_field->ptr == NULL) {
            sa_allocate((SizeableArray**)obj_field->ptr, sizeof(Script));
        }
        sa_set((SizeableArray**)obj_field->ptr, obj_field->idx, &(obj_field->storage));
        break;

    case SA_TYPE_QUEST:
        // Ensure array exists with element size = PcQuestState, then set element at idx.
        if (*(SizeableArray**)obj_field->ptr == NULL) {
            sa_allocate((SizeableArray**)obj_field->ptr, sizeof(PcQuestState));
        }
        sa_set((SizeableArray**)obj_field->ptr, obj_field->idx, &(obj_field->storage));
        break;

    case SA_TYPE_STRING:
        // Replace existing string with a duplicate of storage.str.
        // Ownership: this function owns and frees the previous allocation.
        if (*(char**)obj_field->ptr != NULL) {
            FREE(*(char**)obj_field->ptr);
        }
        *(char**)obj_field->ptr = STRDUP(obj_field->storage.str);
        break;

    case SA_TYPE_HANDLE:
        // Pointer-backed ObjectID.
        // storage.oid.type == OID_TYPE_NULL → free and NULL the target
        // otherwise                        → ensure allocation and copy struct
        if (obj_field->storage.oid.type != OID_TYPE_NULL) {
            if (*(ObjectID**)obj_field->ptr == NULL) {
                *(ObjectID**)obj_field->ptr = MALLOC(sizeof(ObjectID));
            }
            **(ObjectID**)obj_field->ptr = obj_field->storage.oid;
            break; // NOTE: intentional break here to avoid fall-through to the else-free path.
        } else {
            if (*(ObjectID**)obj_field->ptr != NULL) {
                FREE(*(ObjectID**)obj_field->ptr);
                *(ObjectID**)obj_field->ptr = NULL;
            }
        }
        break;

    case SA_TYPE_HANDLE_ARRAY:
        // Ensure array exists with element size = ObjectID, then set at idx.
        if (*(SizeableArray**)obj_field->ptr == NULL) {
            sa_allocate((SizeableArray**)obj_field->ptr, sizeof(ObjectID));
        }
        sa_set((SizeableArray**)obj_field->ptr, obj_field->idx, &(obj_field->storage));
        break;

    case SA_TYPE_PTR:
        // Raw pointer-sized integer copy (no ownership implied).
        *(intptr_t*)obj_field->ptr = obj_field->storage.ptr;
        break;

    case SA_TYPE_PTR_ARRAY:
        // Ensure array exists with element size = intptr_t, then set at idx.
        if (*(SizeableArray**)obj_field->ptr == NULL) {
            sa_allocate((SizeableArray**)obj_field->ptr, sizeof(intptr_t));
        }
        sa_set((SizeableArray**)obj_field->ptr, obj_field->idx, &(obj_field->storage));
        break;
    }
}

// Reads the current live value of an ObjSa field (via pointer or array)
// and stores it into its internal storage buffer.
// This is the inverse of `object_field_apply_from_storage()`.
// 0x4E4180
void object_field_load_into_storage(ObjSa* field)
{
    switch (field->type) {
    case SA_TYPE_INT32:
        // Plain 32-bit integer: copy the value directly from memory.
        field->storage.value = *(int*)field->ptr;
        break;

    case SA_TYPE_INT64: {
        // Pointer-backed 64-bit integer.
        // If a valid int64_t* exists, read the value it points to.
        // Otherwise, store 0 to represent "no value".
        int64_t** int64_handle = (int64_t**)field->ptr;
        if (*int64_handle != NULL) {
            field->storage.value64 = **int64_handle;
        } else {
            field->storage.value64 = 0;
        }
        break;
    }

    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_UINT32_ARRAY: {
        // 32-bit integer arrays: read one element from SizeableArray.
        // If the array exists, fetch element at `field->idx` into storage.
        // Otherwise, store 0.
        SizeableArray** array_ref = (SizeableArray**)field->ptr;
        if (*array_ref != NULL) {
            sa_get(array_ref, field->idx, &field->storage);
        } else {
            field->storage.value = 0;
        }
        break;
    }

    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT64_ARRAY: {
        // 64-bit integer arrays: same logic as above, but with 64-bit storage.
        SizeableArray** array_ref = (SizeableArray**)field->ptr;
        if (*array_ref != NULL) {
            sa_get(array_ref, field->idx, &field->storage);
        } else {
            field->storage.value64 = 0;
        }
        break;
    }

    case SA_TYPE_SCRIPT: {
        // Script arrays: copy one Script struct from the array.
        // If missing, zero the script storage to avoid uninitialized data.
        SizeableArray** array_ref = (SizeableArray**)field->ptr;
        if (*array_ref != NULL) {
            sa_get(array_ref, field->idx, &field->storage);
        } else {
            memset(&field->storage.scr, 0, sizeof(field->storage.scr));
        }
        break;
    }

    case SA_TYPE_QUEST: {
        // Quest arrays: copy one PcQuestState struct.
        // If array is absent, zero the quest structure.
        SizeableArray** array_ref = (SizeableArray**)field->ptr;
        if (*array_ref != NULL) {
            sa_get(array_ref, field->idx, &field->storage);
        } else {
            memset(&field->storage.quest, 0, sizeof(field->storage.quest));
        }
        break;
    }

    case SA_TYPE_STRING: {
        // String: duplicate the current string value into storage.
        // If no string exists, set storage.str = NULL.
        char** str_ref = (char**)field->ptr;
        if (*str_ref != NULL) {
            field->storage.str = STRDUP(*str_ref);
        } else {
            field->storage.str = NULL;
        }
        break;
    }

    case SA_TYPE_HANDLE: {
        // HANDLE = pointer-backed ObjectID.
        // If the handle exists, copy the ObjectID by value.
        // If NULL, mark the stored handle as a null object ID.
        ObjectID** object_id_ptr = (ObjectID**)field->ptr;
        if (*object_id_ptr != NULL) {
            field->storage.oid = **object_id_ptr;
        } else {
            field->storage.oid.type = OID_TYPE_NULL;
        }
        break;
    }

    case SA_TYPE_HANDLE_ARRAY: {
        // Handle arrays: read one ObjectID from SizeableArray.
        // Store a null ObjectID if array does not exist.
        SizeableArray** array_ref = (SizeableArray**)field->ptr;
        if (*array_ref != NULL) {
            sa_get(array_ref, field->idx, &field->storage);
        } else {
            field->storage.oid.type = OID_TYPE_NULL;
        }
        break;
    }

    case SA_TYPE_PTR:
        // Raw pointer type: copy the pointer-sized integer directly.
        field->storage.ptr = *(intptr_t*)field->ptr;
        break;

    case SA_TYPE_PTR_ARRAY: {
        // Pointer arrays (as integers): read one intptr_t from array.
        // Store 0 if array missing.
        SizeableArray** array_ref = (SizeableArray**)field->ptr;
        if (*array_ref != NULL) {
            sa_get(array_ref, field->idx, &field->storage);
        } else {
            field->storage.ptr = 0;
        }
        break;
    }
    }
}


// Deep-copy the current live value of an ObjSa field into a new destination pointer.
// The destination (`value`) receives a copy or duplicate of the data held by `field->ptr`.
// For pointer-backed types, new memory is allocated as needed.
// For arrays, deep copies are made using sub_4E74A0().
// Non-serializable pointer types (SA_TYPE_PTR / PTR_ARRAY) are not supported.
//
// 0x4E4280
void object_field_deep_copy_value(ObjSa* field, void* value)
{
    switch (field->type) {
    case SA_TYPE_INT32:
        // Direct copy of a 32-bit integer.
        *(int*)value = *(int*)field->ptr;
        break;

    case SA_TYPE_INT64:
        // Pointer-backed 64-bit integer.
        // If source pointer is valid, allocate a new int64_t and copy its value.
        // Otherwise, output pointer is set to NULL.
        if (*(int64_t**)field->ptr != NULL) {
            *(int64_t**)value = (int64_t*)MALLOC(sizeof(int64_t));
            **(int64_t**)value = **(int64_t**)field->ptr;
        } else {
            *(int64_t**)value = NULL;
        }
        break;

    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        // Array-based types (SizeableArray-backed).
        // If the source array exists, create a deep copy using sub_4E74A0().
        // Otherwise, set destination pointer to NULL.
        if (*(SizeableArray**)field->ptr != NULL) {
            sub_4E74A0((SizeableArray**)value, (SizeableArray**)field->ptr);
        } else {
            *(SizeableArray**)value = NULL;
        }
        break;

    case SA_TYPE_STRING:
        // String type: duplicate the C string using STRDUP().
        // Destination becomes a new heap-allocated copy or NULL if no string exists.
        if (*(char**)field->ptr != NULL) {
            *(char**)value = STRDUP(*(char**)field->ptr);
        } else {
            *(char**)value = NULL;
        }
        break;

    case SA_TYPE_HANDLE:
        // Pointer-backed ObjectID.
        // If handle exists, allocate a new ObjectID and copy the struct.
        // Otherwise, set destination pointer to NULL.
        if (*(ObjectID**)field->ptr != NULL) {
            *(ObjectID**)value = (ObjectID*)MALLOC(sizeof(ObjectID));
            **(ObjectID**)value = **(ObjectID**)field->ptr;
        } else {
            *(ObjectID**)value = NULL;
        }
        break;

    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        // Raw pointer or pointer-array types are intentionally unsupported.
        // These do not have defined deep-copy semantics.
        assert(0);
    }
}

// Reads an ObjSa field's serialized value from a TigFile into its live in-memory pointer.
// Allocates or frees memory for pointer-backed and array types as needed.
//
// Format overview per type:
// - Primitive types (INT32): raw binary values.
// - Pointer-backed types (INT64, HANDLE, STRING): prefixed with a 1-byte presence flag.
// - Arrays and structured types: prefixed with presence byte, then serialized data.
//
// Returns true on success, false on any read error.
//
// 0x4E4360
bool object_field_read_from_file(ObjSa* field, TigFile* file)
{
    uint8_t has_value; // 1 = value present, 0 = null/absent
    int string_len; // used for SA_TYPE_STRING

    switch (field->type) {
    case SA_TYPE_INT32:
        // Plain 32-bit integer: directly read into memory.
        if (!objf_read(field->ptr, sizeof(int), file)) {
            return false;
        }
        return true;

    case SA_TYPE_INT64:
        // Pointer-backed 64-bit integer.
        // Format: [has_value:1][value:8 if present]
        if (!objf_read(&has_value, sizeof(has_value), file)) {
            return false;
        }
        if (!has_value) {
            if (*(int64_t**)field->ptr != NULL) {
                FREE(*(int64_t**)field->ptr);
                *(int64_t**)field->ptr = NULL;
            }
            return true;
        }
        if (*(int64_t**)field->ptr == NULL) {
            *(int64_t**)field->ptr = (int64_t*)MALLOC(sizeof(int64_t));
        }
        if (!objf_read(*(int64_t**)field->ptr, sizeof(int64_t), file)) {
            return false;
        }
        return true;

    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        // Array-backed types (SizeableArray-based).
        // Format: [has_value:1][array data if present]
        if (!objf_read(&has_value, sizeof(has_value), file)) {
            return false;
        }
        if (!has_value) {
            *(SizeableArray**)field->ptr = NULL;
            return true;
        }
        if (!sa_read((SizeableArray**)field->ptr, file)) {
            return false;
        }
        return true;

    case SA_TYPE_STRING:
        // Variable-length string.
        // Format: [has_value:1][length:4][data:length+1 bytes including null terminator]
        if (!objf_read(&has_value, sizeof(has_value), file)) {
            return false;
        }
        if (*(char**)field->ptr != NULL) {
            FREE(*(char**)field->ptr);
        }
        if (!has_value) {
            *(char**)field->ptr = NULL;
            return true;
        }
        if (!objf_read(&string_len, sizeof(string_len), file)) {
            return false;
        }
        *(char**)field->ptr = (char*)MALLOC(string_len + 1);
        if (!objf_read(*(char**)field->ptr, string_len + 1, file)) {
            return false;
        }
        return true;

    case SA_TYPE_HANDLE:
        // Pointer-backed ObjectID structure.
        // Format: [has_value:1][ObjectID struct if present]
        if (!objf_read(&has_value, sizeof(has_value), file)) {
            return false;
        }
        if (!has_value) {
            if (*(ObjectID**)field->ptr != NULL) {
                FREE(*(ObjectID**)field->ptr);
                *(ObjectID**)field->ptr = NULL;
            }
            return true;
        }
        if (*(ObjectID**)field->ptr == NULL) {
            *(ObjectID**)field->ptr = (ObjectID*)MALLOC(sizeof(ObjectID));
        }
        if (!objf_read(*(ObjectID**)field->ptr, sizeof(ObjectID), file)) {
            return false;
        }
        return true;

    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        // Pointer types are not serialized — invalid to encounter here.
        assert(0);

    default:
        // Unknown or unsupported field type.
        return false;
    }
}

// Reads a serialized ObjSa field value from a TigFile into its in-memory pointer,
// without freeing any existing allocations beforehand.
// This "no-dealloc" variant assumes the destination is uninitialized
// or safe to overwrite directly.
//
// Format overview per type:
// - INT32: raw 4-byte value.
// - Pointer-backed types (INT64, HANDLE, STRING): prefixed with 1-byte has_value flag.
// - Arrays and structured types: prefixed with has_value byte, followed by serialized data.
// - Pointer types (PTR / PTR_ARRAY) are unsupported.
//
// Returns true on success, false on any read or format error.
//
// 0x4E44F0
bool object_field_read_from_file_no_dealloc(ObjSa* field, TigFile* file)
{
    uint8_t has_value; // 1 = value present, 0 = absent
    int string_len; // used for SA_TYPE_STRING length field

    switch (field->type) {

    // ───────────────────────────────
    // Primitive 32-bit integer
    // ───────────────────────────────
    case SA_TYPE_INT32:
        // Read the raw integer directly into memory.
        if (!objf_read(field->ptr, sizeof(int), file)) {
            return false;
        }
        return true;

    // ───────────────────────────────
    // Pointer-backed 64-bit integer
    // ───────────────────────────────
    case SA_TYPE_INT64:
        // Read has_value flag.
        if (!objf_read(&has_value, sizeof(has_value), file)) {
            return false;
        }
        // If absent, set pointer to NULL.
        if (!has_value) {
            *(int64_t**)field->ptr = NULL;
            return true;
        }
        // Otherwise allocate and read value.
        *(int64_t**)field->ptr = (int64_t*)MALLOC(sizeof(int64_t));
        if (!objf_read(*(int64_t**)field->ptr, sizeof(int64_t), file)) {
            return false;
        }
        return true;

    // ───────────────────────────────
    // Array-backed and structured types
    // ───────────────────────────────
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        // Read has_value flag to know if array exists.
        if (!objf_read(&has_value, sizeof(has_value), file)) {
            return false;
        }
        // If no array, set pointer to NULL.
        if (!has_value) {
            *(SizeableArray**)field->ptr = NULL;
            return true;
        }
        // Otherwise read array without deallocating existing memory.
        if (!sa_read_no_dealloc((SizeableArray**)field->ptr, file)) {
            return false;
        }
        return true;

    // ───────────────────────────────
    // Strings
    // ───────────────────────────────
    case SA_TYPE_STRING:
        // Read has_value flag.
        if (!objf_read(&has_value, sizeof(has_value), file)) {
            return false;
        }
        // If string missing, set NULL.
        if (!has_value) {
            *(char**)field->ptr = NULL;
            return true;
        }
        // Otherwise read string length and contents.
        if (!objf_read(&string_len, sizeof(string_len), file)) {
            return false;
        }
        *(char**)field->ptr = (char*)MALLOC(string_len + 1);
        if (!objf_read(*(char**)field->ptr, string_len + 1, file)) {
            return false;
        }
        return true;

    // ───────────────────────────────
    // Object handles
    // ───────────────────────────────
    case SA_TYPE_HANDLE:
        // Read has_value flag.
        if (!objf_read(&has_value, sizeof(has_value), file)) {
            return false;
        }
        // If absent, set pointer to NULL.
        if (!has_value) {
            *(ObjectID**)field->ptr = NULL;
            return true;
        }
        // Otherwise allocate and read ObjectID struct.
        *(ObjectID**)field->ptr = (ObjectID*)MALLOC(sizeof(ObjectID));
        if (!objf_read(*(ObjectID**)field->ptr, sizeof(ObjectID), file)) {
            return false;
        }
        return true;

    // ───────────────────────────────
    // Non-serializable pointer types
    // ───────────────────────────────
    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        // These should never be serialized; abort if encountered.
        assert(0);

    // ───────────────────────────────
    // Unknown or unsupported type
    // ───────────────────────────────
    default:
        return false;
    }
}


// Deserialize a single ObjSa field from an in-memory byte buffer (`cursor`) into its live storage.
//
// Format conventions by type:
// - SA_TYPE_INT32: raw 4 bytes copied into target.
// - SA_TYPE_INT64 / SA_TYPE_HANDLE / arrays / string:
//     Leading 1-byte `has_value` flag.
//       has_value == 0 → free existing allocation (if any) and set pointer to NULL.
//       has_value == 1 → ensure allocation (for pointer-backed), then read payload.
// - Arrays use sub_4E7820() to read their payload.
// - SA_TYPE_PTR / SA_TYPE_PTR_ARRAY are unsupported here (assert).
//
// Advances *cursor by the number of bytes consumed.
// Leaves the destination field either NULL or a valid allocation.
//
// 0x4E4660
void object_field_read_from_memory(ObjSa* field, uint8_t** cursor)
{
    uint8_t has_value; // 1 → value present; 0 → null/absent
    int strlen_bytes; // SA_TYPE_STRING: number of bytes excluding the NUL

    switch (field->type) {

    // Plain 32-bit integer: copy 4 bytes into the target location.
    case SA_TYPE_INT32:
        memory_read_from_cursor(field->ptr, sizeof(int), cursor);
        return;

    // Pointer-backed 64-bit integer:
    // Layout: [has_value:1][int64 payload if has_value==1]
    case SA_TYPE_INT64:
        memory_read_from_cursor(&has_value, sizeof(has_value), cursor);
        if (!has_value) {
            if (*(int64_t**)field->ptr != NULL) {
                FREE(*(int64_t**)field->ptr);
                *(int64_t**)field->ptr = NULL;
            }
            return;
        }
        if (*(int64_t**)field->ptr == NULL) {
            *(int64_t**)field->ptr = (int64_t*)MALLOC(sizeof(int64_t));
        }
        memory_read_from_cursor(*(int64_t**)field->ptr, sizeof(int64_t), cursor);
        return;

    // SizeableArray-backed types:
    // Layout: [has_value:1][array payload if has_value==1 via sub_4E7820]
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        memory_read_from_cursor(&has_value, sizeof(has_value), cursor);
        if (!has_value) {
            if (*(SizeableArray**)field->ptr != NULL) {
                FREE(*(SizeableArray**)field->ptr);
                *(SizeableArray**)field->ptr = NULL;
            }
            return;
        }
        sub_4E7820((SizeableArray**)field->ptr, cursor);
        return;

    // String:
    // Layout: [has_value:1][strlen_bytes:int32][bytes:strlen_bytes+1 including NUL]
    case SA_TYPE_STRING:
        memory_read_from_cursor(&has_value, sizeof(has_value), cursor);
        if (*(char**)field->ptr != NULL) {
            FREE(*(char**)field->ptr);
        }
        if (!has_value) {
            *(char**)field->ptr = NULL;
            return;
        }
        memory_read_from_cursor(&strlen_bytes, sizeof(strlen_bytes), cursor);
        *(char**)field->ptr = (char*)MALLOC(strlen_bytes + 1);
        memory_read_from_cursor(*(char**)field->ptr, strlen_bytes + 1, cursor);
        return;

    // Pointer-backed ObjectID:
    // Layout: [has_value:1][ObjectID payload if has_value==1]
    case SA_TYPE_HANDLE:
        memory_read_from_cursor(&has_value, sizeof(has_value), cursor);
        if (!has_value) {
            if (*(ObjectID**)field->ptr != NULL) {
                FREE(*(ObjectID**)field->ptr);
                *(ObjectID**)field->ptr = NULL;
            }
            return;
        }
        if (*(ObjectID**)field->ptr == NULL) {
            *(ObjectID**)field->ptr = (ObjectID*)MALLOC(sizeof(ObjectID));
        }
        memory_read_from_cursor(*(ObjectID**)field->ptr, sizeof(ObjectID), cursor);
        return;

    // Non-serializable pointer types: should not appear here.
    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        assert(0);
    }
}

// Serialize a single ObjSa field to a TigFile stream.
//
// Each field type uses a specific binary format:
// - SA_TYPE_INT32:
//     Writes the 4-byte integer directly.
// - SA_TYPE_INT64 / SA_TYPE_HANDLE / SA_TYPE_STRING / Array-backed types:
//     Start with a 1-byte presence flag (1 = value present, 0 = null/absent).
//     If present, write the data payload (struct, string, or array).
// - Strings include a 4-byte length field followed by the string bytes + '\0'.
// - Arrays (SizeableArray-backed) are serialized via sa_write().
// - SA_TYPE_PTR / SA_TYPE_PTR_ARRAY are unsupported (assert).
//
// Returns true on success, false if any write fails.
//
// 0x4E47E0
bool object_field_write_to_file(ObjSa* field, TigFile* file)
{
    uint8_t presence; // 1 = value present, 0 = null/absent
    int size; // Used for string length

    switch (field->type) {

    // Plain 32-bit integer: write directly.
    case SA_TYPE_INT32:
        if (!objf_write((int*)field->ptr, sizeof(int), file))
            return false;
        return true;

    // Pointer-backed 64-bit integer:
    // Format: [presence:1][value:8 if present]
    case SA_TYPE_INT64:
        if (*(int64_t**)field->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), file)) return false;
            if (!objf_write(*(int64_t**)field->ptr, sizeof(int64_t), file)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), file)) return false;
        }
        return true;

    // SizeableArray-backed types:
    // Format: [presence:1][array payload if present]
    // Uses sa_write() to serialize the array contents.
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        if (*(SizeableArray**)field->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), file)) return false;
            if (!sa_write((SizeableArray**)field->ptr, file)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), file)) return false;
        }
        return true;

    // String:
    // Format: [presence:1][length:int32][data:length+1 (including NUL)]
    case SA_TYPE_STRING:
        if (*(char**)field->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), file)) return false;
            size = (int)strlen(*(char**)field->ptr);
            if (!objf_write(&size, sizeof(size), file)) return false;
            if (!objf_write(*(char**)field->ptr, size + 1, file)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), file)) return false;
        }
        return true;

    // Pointer-backed ObjectID:
    // Format: [presence:1][ObjectID struct if present]
    case SA_TYPE_HANDLE:
        if (*(ObjectID**)field->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), file)) return false;
            if (!objf_write(*(ObjectID**)field->ptr, sizeof(ObjectID), file)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), file)) return false;
        }
        return true;

    // Unsupported pointer-based types.
    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        assert(0);

    // Unknown / unhandled field type.
    default:
        return false;
    }
}

// 0x4E4990
void sub_4E4990(ObjSa* a1, MemoryWriteBuffer* a2)
{
    // uint8_t presence;
    // int size;

    // switch (a1->type) {
    // case 3:
    //     sub_4E4C00(a1->d.int_value, sizeof(a1->d.int_value), a2);
    //     break;
    // case 4:
    //     if (*a1->d.b != NULL) {
    //         presence = 1;
    //         sub_4E4C00(&presence, sizeof(presence), a2);
    //         sub_4E4C00(*a1->d.b, sizeof(*a1->d.b), a2);
    //     } else {
    //         presence = 0;
    //         sub_4E4C00(&presence, sizeof(presence), a2);
    //     }
    //     break;
    // case 5:
    // case 6:
    // case 7:
    // case 8:
    // case 9:
    // case 10:
    // case 13:
    //     if (*a1->d.a.sa_ptr != NULL) {
    //         presence = 1;
    //         sub_4E4C00(&presence, sizeof(presence), a2);
    //         size = sub_4E77B0(a1->d.a.sa_ptr);
    //         sub_4E4C80(a2, size);
    //         sub_4E77E0(a1->d.a.sa_ptr, a2->field_4);
    //         a2->field_4 += size;
    //         a2->field_C -= size;
    //     } else {
    //         presence = 0;
    //         sub_4E4C00(&presence, sizeof(presence), a2);
    //     }
    //     break;
    // case 11:
    //     if (*a1->d.str != NULL) {
    //         presence = 1;
    //         sub_4E4C00(&presence, sizeof(presence), a2);
    //         size = strlen(*a1->d.str);
    //         sub_4E4C00(&size, sizeof(size), a2);
    //         sub_4E4C00(*a1->d.str, size + 1, a2);
    //     } else {
    //         presence = 0;
    //         sub_4E4C00(&presence, sizeof(presence), a2);
    //     }
    //     break;
    // case 12:
    //     if (*a1->d.oid != NULL) {
    //         presence = 1;
    //         sub_4E4C00(&presence, sizeof(presence), a2);
    //         sub_4E4C00(*a1->d.oid, sizeof(**a1->d.oid), a2);
    //     } else {
    //         presence = 0;
    //         sub_4E4C00(&presence, sizeof(presence), a2);
    //     }
    //     break;
    // }
}

// Remove an element from a SizeableArray-backed ObjSa field at the given index.
//
// Supported types: all *_ARRAY, SCRIPT, QUEST, and PTR_ARRAY variants.
// If the array exists, removes the element at field->idx via sub_4E7760().
// If the array pointer is NULL, does nothing.
//
// This acts as a safe "remove-at-index" helper for dynamic ObjSa arrays.
//
// 0x4E4B70
void object_field_array_remove_element(ObjSa* field)
{
    switch (field->type) {
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
    case SA_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)field->ptr != NULL) {
            // Remove element at index field->idx.
            sub_4E7760((SizeableArray**)field->ptr, field->idx);
        }
        break;
    }
}

// Return the number of elements in a SizeableArray-backed ObjSa field.
//
// Supported types: all *_ARRAY, SCRIPT, QUEST, and PTR_ARRAY variants.
// If the array pointer is NULL, returns 0.
// Otherwise, returns the element count via sa_count().
//
// 0x4E4BA0
int object_field_array_get_count(ObjSa* field)
{
    switch (field->type) {
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
    case SA_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)field->ptr != NULL) {
            // Return number of elements in the array.
            return sa_count((SizeableArray**)field->ptr);
        }
        break;
    }

    // No array or unsupported type → return 0.
    return 0;
}

// Initialize a MemoryWriteBuffer with a default capacity of 256 bytes.
//
// Allocates an initial heap buffer, sets the write pointer to the start,
// and initializes tracking fields for total and remaining capacity.
//
// Used before writing serialized data into memory.
//
// 0x4E4BD0
void memory_write_buffer_init(MemoryWriteBuffer* buffer)
{
    buffer->base_pointer = (uint8_t*)MALLOC(256); // Allocate initial 256-byte buffer
    buffer->write_pointer = buffer->base_pointer; // Start writing at the beginning
    buffer->total_capacity = 256; // Total allocated size
    buffer->remaining_capacity = buffer->total_capacity; // All capacity unused
}

// Append data to a MemoryWriteBuffer, expanding it if necessary.
//
// Ensures there is enough space for `size` bytes, then copies them from `data`
// into the buffer at the current write position. Advances the write pointer
// and updates the remaining capacity.
//
// 0x4E4C00
void memory_write_buffer_append(const void* data, int size, MemoryWriteBuffer* buffer)
{
    ensure_memory_capacity(buffer, size); // Expand if needed
    memcpy(buffer->write_pointer, data, size); // Copy data to buffer
    buffer->write_pointer += size; // Advance cursor
    buffer->remaining_capacity -= size; // Update remaining space
}

// Read raw bytes from a byte-stream cursor into a destination buffer.
//
// Copies `size` bytes from *cursor into `dest` and advances the source pointer.
// Used for deserializing in-memory data streams.
//
// 0x4E4C50
void memory_read_from_cursor(void* dest, int size, uint8_t** cursor)
{
    memcpy(dest, *cursor, size); // Copy bytes into destination
    (*cursor) += size; // Advance read cursor
}

// Ensure that a MemoryWriteBuffer has enough free capacity for `required` bytes.
//
// If the buffer lacks sufficient remaining capacity, it expands the buffer
// in 256-byte increments until it can accommodate the new data.
// The function then updates all relevant pointers and capacity fields.
//
// Growth policy:
// - Always round up to the next multiple of 256 bytes.
// - Keeps existing data intact via REALLOC.
// - Preserves the current write position.
//
// 0x4E4C80
void ensure_memory_capacity(MemoryWriteBuffer* buffer, int required)
{
    // Calculate how many more bytes are needed beyond current capacity.
    int deficit = required - buffer->remaining_capacity;

    // Only expand when necessary.
    if (deficit > 0) {
        // Round up to the nearest multiple of 256 bytes.
        int grow = ((deficit / 256) + 1) * 256;

        // Increase total capacity and reallocate the underlying buffer.
        buffer->total_capacity += grow;
        buffer->base_pointer = (uint8_t*)REALLOC(buffer->base_pointer, buffer->total_capacity);

        // Recalculate write_pointer to point to the same position relative to new base.
        buffer->write_pointer = buffer->base_pointer
            + buffer->total_capacity
            - buffer->remaining_capacity
            - grow;

        // Update available space after expansion.
        buffer->remaining_capacity += grow;
    }
}

// Initialize global storage and lookup tables used for object field metadata.
//
// This function allocates and initializes all global buffers and helper tables
// used by the field metadata system, including:
// - FieldMetaTable:     stores metadata entries for bitfields
// - FreeFieldMetaIndices: free-list for released metadata indices
// - FieldBitData:       contiguous bitfield data for all metadata entries
// - Popcount16Lookup:   precomputed bit-count table for 16-bit values
// - BitMaskTable:       precomputed bitmask pairs for efficient partial bit ops
//
// Default sizes:
// - FieldMetaCapacity:     4096 entries
// - FreeIndexCapacity:     4096 entries
// - FieldBitDataCapacity:  8192 32-bit words
//
// After allocation, initializes lookup tables via InitPopCountLookup()
// and ObjBitMaskTable_Init(), and marks storage as allocated.
//
// 0x4E59B0
void obj_field_metadata_system_init()
{
    // Reset counters and capacities for field metadata management.
    FieldMetaCount = 0;
    FieldMetaCapacity = 4096;
    FreeIndexCount = 0;
    FreeIndexCapacity = 4096;
    FieldBitDataSize = 0;
    FieldBitDataCapacity = 8192;

    // Allocate core metadata structures.
    FieldMetaTable = (ObjFieldMeta*)MALLOC(sizeof(*FieldMetaTable) * FieldMetaCapacity);
    FreeFieldMetaIndices = (int*)MALLOC(sizeof(*FreeFieldMetaIndices) * FreeIndexCapacity);
    FieldBitData = (int*)MALLOC(sizeof(*FieldBitData) * FieldBitDataCapacity);

    // Allocate lookup tables.
    Popcount16Lookup = (uint8_t*)MALLOC(65536); // 2^16 entries
    BitMaskTable = (BitMaskPair*)MALLOC(sizeof(*BitMaskTable) * 33); // Masks for 0–32 bits

    // Initialize lookup data.
    InitPopCountLookup(); // Builds popcount lookup for 16-bit integers.
    ObjBitMaskTable_Init(); // Builds bitmask table for partial bit operations.

    // Mark system as initialized.
    ObjPrivateStorageAllocated = true;
}

// Free all global allocations used by the object field metadata system.
//
// This function releases all memory allocated by obj_field_metadata_system_init(),
// including lookup tables, bitfield data, and metadata arrays.
// It resets the system’s allocation flag to indicate that no metadata
// storage is currently active.
//
// Call this during shutdown or when reinitializing object field metadata.
//
// 0x4E5A50
void obj_field_metadata_system_shutdown()
{
    // Free lookup tables.
    FREE(BitMaskTable);
    FREE(Popcount16Lookup);

    // Free bitfield and metadata storage arrays.
    FREE(FieldBitData);
    FREE(FreeFieldMetaIndices);
    FREE(FieldMetaTable);

    // Mark system as deallocated.
    ObjPrivateStorageAllocated = false;
}


// 0x4E5AA0
int field_metadata_acquire()
{
    int index;

    if (FreeIndexCount != 0) {
        return FreeFieldMetaIndices[--FreeIndexCount];
    }

    if (FieldMetaCount == FieldMetaCapacity) {
        FieldMetaCapacity += 4096;
        FieldMetaTable = (ObjFieldMeta*)REALLOC(FieldMetaTable, sizeof(ObjFieldMeta) * FieldMetaCapacity);
    }

    index = FieldMetaCount++;
    if (index == 0) {
        FieldMetaTable[0].word_offset = index;
    } else {
        FieldMetaTable[index].word_offset = FieldMetaTable[index - 1].word_count + FieldMetaTable[index - 1].word_offset;
    }

    FieldMetaTable[index].word_count = 0;
    field_metadata_grow_word_array(index, 2);

    return index;
}

// 0x4E5B40
int field_metadata_release(int a1)
{
    ObjFieldMeta* v1;
    int index;

    v1 = &(FieldMetaTable[a1]);
    if (v1->word_count != 2) {
        field_metadata_shrink_word_array(a1, v1->word_count - 2);
    }

    for (index = v1->word_offset; index < v1->word_count + v1->word_offset; index++) {
        FieldBitData[index] = 0;
    }

    if (FreeIndexCount == FreeIndexCapacity) {
        FreeIndexCapacity += 4096;
        FreeFieldMetaIndices = (int*)REALLOC(FreeFieldMetaIndices, sizeof(int) * FreeIndexCapacity);
    }

    FreeFieldMetaIndices[FreeIndexCount] = a1;

    return ++FreeIndexCount;
}

// 0x4E5BF0
int field_metadata_clone(int a1)
{
    int v1;
    int v2;
    int index;

    v1 = field_metadata_acquire();
    v2 = FieldMetaTable[a1].word_count - FieldMetaTable[v1].word_count;
    if (v2 != 0) {
        field_metadata_grow_word_array(v1, v2);
    }

    for (index = 0; index < FieldMetaTable[a1].word_count; index++) {
        FieldBitData[FieldMetaTable[v1].word_offset + index] = FieldBitData[FieldMetaTable[a1].word_offset + index];
    }

    return v1;
}

// 0x4E5C60
void field_metadata_set_or_clear_bit(int a1, int a2, bool a3)
{
    int v1;
    int v2;

    v1 = convert_bit_index_to_word_index(a2);
    if (v1 >= FieldMetaTable[a1].word_count) {
        if (!a3) {
            return;
        }

        field_metadata_grow_word_array(a1, v1 - FieldMetaTable[a1].word_count + 1);
    }

    v2 = convert_bit_index_to_mask(a2);
    if (a3) {
        FieldBitData[v1 + FieldMetaTable[a1].word_offset] |= v2;
    } else {
        FieldBitData[v1 + FieldMetaTable[a1].word_offset] &= ~v2;
    }
}

// 0x4E5CE0
int field_metadata_test_bit(int a1, int a2)
{
    int v1;
    int v2;

    v1 = convert_bit_index_to_word_index(a2);
    if (v1 > FieldMetaTable[a1].word_count - 1) {
        return 0;
    }

    v2 = convert_bit_index_to_mask(a2);
    return v2 & FieldBitData[v1 + FieldMetaTable[a1].word_offset];
}

// 0x4E5D30
// Count the number of set bits in a field metadata entry for all bit positions
// strictly less than `bit_index_limit`.
//
// Semantics:
// - Exclusive upper bound: bits with index < bit_index_limit are counted.
// - If bit_index_limit lies inside an existing word, we partially count that word's
//   lower (bit_index_limit % 32) bits.
// - If bit_index_limit is beyond the current word count, we count all existing words.
//
// Complexity: O(number of 32-bit words touched).
int field_metadata_count_set_bits_up_to(int metadata_index, int bit_index_limit)
{
    int total_count = 0;

    // Start of this entry's bitfield in the global word array.
    int base_word_index = FieldMetaTable[metadata_index].word_offset;

    // Word index that contains the (exclusive) upper-bound bit.
    int limit_word_index = convert_bit_index_to_word_index(bit_index_limit);

    // Absolute index into the global array of the limit word for this entry.
    int stop_word_index = base_word_index + limit_word_index;

    // If the limit word exists, count only the lower (bit_index_limit % 32) bits of it.
    // This yields exclusive semantics: we do not include the bit at bit_index_limit.
    if (limit_word_index < FieldMetaTable[metadata_index].word_count) {
        total_count += count_set_bits_in_word_up_to_limit(
            FieldBitData[stop_word_index],
            bit_index_limit % 32 // number of lower bits to include (0..31)
        );
    } else {
        // If the limit is beyond the current words, cap at the end of this entry's region.
        stop_word_index = base_word_index + FieldMetaTable[metadata_index].word_count;
    }

    // Count all full words strictly before the limit word.
    while (base_word_index < stop_word_index) {
        total_count += count_set_bits_in_word_up_to_limit(FieldBitData[base_word_index++], 32);
    }

    return total_count;
}

// Iterate all set bits in the bitfield for metadata entry `a1` and call `callback(pos)`.
//
// Semantics:
// - Visits bits in ascending order (0,1,2,...).
// - For each set bit, calls `callback(bit_index_relative_to_entry)`.
// - If the callback returns false, iteration stops and the function returns false.
// - Returns true only if the entire bitfield is scanned without an early stop.
//
// Implementation details:
// - The bitfield is stored as a range of 32-bit words inside the global FieldBitData.
// - `word_offset` is the start index in FieldBitData; `word_count` is the number of words.
// - We scan each word and test its 32 bits using a rolling 1-bit mask.
//
// Complexity: O(number_of_words * 32).
bool field_metadata_iterate_set_bits(int a1, bool (*callback)(int))
{
    int start_word = FieldMetaTable[a1].word_offset;
    int end_word = FieldMetaTable[a1].word_count + start_word;

    int pos = 0; // bit index relative to the start of this entry's bitfield

    for (int idx = start_word; idx < end_word; idx++) {
        unsigned int mask = 1;
        unsigned int word = FieldBitData[idx];

        for (int bit = 0; bit < 32; bit++) {
            if (word & mask) {
                if (!callback(pos)) {
                    return false; // early exit on callback failure
                }
            }
            mask <<= 1;
            pos++;
        }
    }

    return true;
}

// Serialize a field metadata entry to a TigFile stream.
//
// Writes the bitfield's word count followed by its raw 32-bit word data
// from the global FieldBitData array.
//
// Binary format:
//   [word_count: int32]
//   [bit_data:   int32[word_count]]
//
// Returns:
//   true  – if both write operations succeed.
//   false – if any write fails.
//
// 0x4E5E20
bool field_metadata_serialize_to_tig_file(int metadata_index, TigFile* stream)
{
    // Write the number of 32-bit words in this metadata entry.
    if (tig_file_fwrite(&(FieldMetaTable[metadata_index].word_count), sizeof(int), 1, stream) != 1) {
        return false;
    }

    // Write the actual bitfield data segment corresponding to this entry.
    if (tig_file_fwrite(
            &(FieldBitData[FieldMetaTable[metadata_index].word_offset]),
            sizeof(int) * FieldMetaTable[metadata_index].word_count,
            1,
            stream)
        != 1) {
        return false;
    }

    return true;
}

// Deserialize a field metadata entry from a TigFile stream.
//
// Reads a serialized bitfield entry previously written by
// field_metadata_serialize_to_tig_file() and reconstructs it
// into a new entry within the global metadata tables.
//
// Binary format:
//   [word_count: int32]
//   [bit_data:   int32[word_count]]
//
// Behavior:
// - Allocates a new metadata entry using field_metadata_acquire().
// - Expands its word array if the stored word_count exceeds the default size.
// - Reads the bit data directly into the global FieldBitData buffer.
// - Stores the new entry index into *out_index.
//
// Returns:
//   true  – on successful read and reconstruction.
//   false – on any read failure.
//
// 0x4E5E80
bool field_metadata_deserialize_from_tig_file(int* out_index, TigFile* stream)
{
    int metadata_index;
    int word_count;

    // Acquire a new metadata entry slot.
    metadata_index = field_metadata_acquire();

    // Read the number of 32-bit words in this serialized entry.
    if (tig_file_fread(&word_count, sizeof(word_count), 1, stream) != 1) {
        return false;
    }

    // Expand this entry’s storage if the serialized size is larger.
    if (word_count > FieldMetaTable[metadata_index].word_count) {
        field_metadata_grow_word_array(metadata_index, word_count - FieldMetaTable[metadata_index].word_count);
    }

    // Read the serialized bitfield data directly into the allocated space.
    if (tig_file_fread(
            &(FieldBitData[FieldMetaTable[metadata_index].word_offset]),
            sizeof(int) * word_count,
            1,
            stream)
        != 1) {
        return false;
    }

    // Return the index of the newly reconstructed metadata entry.
    *out_index = metadata_index;

    return true;
}

// Calculate the number of bytes required to export a field metadata entry.
//
// The exported format is identical to the one used in serialization:
//   [word_count: int32]
//   [bit_data:   int32[word_count]]
//
// So the total size is 4 bytes for the count + 4 bytes per 32-bit word.
//
// 0x4E5F10
int field_metadata_calculate_export_size(int metadata_index)
{
    return sizeof(int) * (FieldMetaTable[metadata_index].word_count + 1);
}

// Export a field metadata entry into a contiguous memory buffer.
//
// Writes the word count followed by the raw bit data for the given
// metadata entry. This is used when serializing in-memory structures
// (e.g., saving object state to memory instead of disk).
//
// Layout:
//   [word_count: int32]
//   [bit_data:   int32[word_count]]
//
// Caller must ensure that `buffer` is large enough, typically using
// field_metadata_calculate_export_size().
//
// 0x4E5F30
void field_metadata_export_to_memory(int metadata_index, void* buffer)
{
    int word_offset = FieldMetaTable[metadata_index].word_offset;
    int word_count = FieldMetaTable[metadata_index].word_count;

    // Write word count.
    *(int*)buffer = word_count;

    // Write bitfield data immediately after.
    memcpy((int*)buffer + 1, &(FieldBitData[word_offset]), sizeof(int) * word_count);
}

// Import a field metadata entry from a raw memory buffer.
//
// Reconstructs a bitfield metadata entry previously exported via
// field_metadata_export_to_memory(). The format must match:
//
//   [word_count: int32]
//   [bit_data:   int32[word_count]]
//
// Allocates a new entry, grows its internal storage if needed,
// and copies data from the byte stream.
//
// 0x4E5F70
void field_metadata_import_from_memory(int* out_index, uint8_t** cursor)
{
    int metadata_index;
    int word_count;

    // Create a new metadata entry.
    metadata_index = field_metadata_acquire();

    // Read the number of 32-bit words from the input stream.
    memory_read_from_cursor(&word_count, sizeof(word_count), cursor);

    // Expand the entry’s word array if necessary.
    if (word_count > FieldMetaTable[metadata_index].word_count) {
        field_metadata_grow_word_array(metadata_index, word_count - FieldMetaTable[metadata_index].word_count);
    }

    // Read the bitfield data directly into the allocated memory.
    memory_read_from_cursor(&(FieldBitData[FieldMetaTable[metadata_index].word_offset]), sizeof(int) * word_count, cursor);

    // Return the index of the reconstructed metadata entry.
    *out_index = metadata_index;
}

// Count the number of set bits in the lower `limit_bits` of a 32-bit word.
//
// Semantics:
// - Only the least-significant `limit_bits` (0..32) of `word` are considered.
// - Bits above `limit_bits - 1` are ignored.
// - Implemented via two 16-bit popcount lookups using precomputed masks.
//
// Implementation details:
// - `BitMaskTable[limit_bits]` provides two 16-bit masks:
//     * lower_mask: mask for the low 16 bits
//     * upper_mask: mask for the high 16 bits (shifted part of the lower N bits)
// - `Popcount16Lookup[v]` is the popcount for any 16-bit value `v` (0..65535).
// - We split `word` into low/high 16-bit halves, apply the corresponding masks,
//   then sum their 16-bit popcounts.
//
// Constraints:
// - `limit_bits` must be in [0, 32]. (Table is initialized for 0..32.)
//
// 0x4E5FE0
int count_set_bits_in_word_up_to_limit(int word, int limit_bits)
{
    return Popcount16Lookup[BitMaskTable[limit_bits].lower_mask & (word & 0xFFFF)]
        + Popcount16Lookup[BitMaskTable[limit_bits].upper_mask & ((word >> 16) & 0xFFFF)];
}

// 0x4E6040
// Grow the bitfield (word array) for a given field metadata entry by
// `additional_word_count` 32-bit words.
//
// This function may:
// - Expand the global backing storage (FieldBitData) in chunks of 8192 words.
// - Shift later segments forward to make room for the newly inserted words.
// - Update start offsets for all later segments.
// - Zero-initialize the newly added words for this metadata entry.
//
// Complexity notes:
// - Potential O(N) memmove when this entry is not the last segment.
// - Capacity growth uses geometric-style chunking to limit reallocations.
void field_metadata_grow_word_array(int metadata_index, int additional_word_count)
{
    int* next_segment_data;
    size_t words_to_move;
    int zero_start_index;
    int zero_end_index;
    int data_index;

    // Ensure the global backing array has enough capacity.
    if (FieldBitDataSize + additional_word_count > FieldBitDataCapacity) {
        // Grow capacity in 8192-word chunks.
        FieldBitDataCapacity += ((FieldBitDataSize + additional_word_count - FieldBitDataCapacity - 1) / 8192 + 1) * 8192;
        FieldBitData = REALLOC(FieldBitData, sizeof(*FieldBitData) * FieldBitDataCapacity);
    }

    // If this metadata entry is not the last one, shift subsequent segments forward
    // to make room for the words we are inserting into this entry.
    if (metadata_index != FieldMetaCount - 1) {
        // Pointer to the start of the next segment's data in the global array.
        next_segment_data = &(FieldBitData[FieldMetaTable[metadata_index + 1].word_offset]);

        // Number of words occupied by all segments after the next segment's start.
        words_to_move = (size_t)(FieldBitDataSize - FieldMetaTable[metadata_index + 1].word_offset);

        // Move later data forward by `additional_word_count` words.
        memmove(&(next_segment_data[additional_word_count]),
            next_segment_data,
            sizeof(*next_segment_data) * words_to_move);

        // Update the recorded start offsets for all segments after this one.
        field_metadata_adjust_offsets(metadata_index + 1,
            FieldMetaCount - 1,
            additional_word_count);
    }

    // Increase this entry's word count and the global size of occupied words.
    FieldMetaTable[metadata_index].word_count += additional_word_count;
    FieldBitDataSize += additional_word_count;

    // Zero-initialize the newly inserted words for this entry.
    zero_start_index = FieldMetaTable[metadata_index].word_offset
        + FieldMetaTable[metadata_index].word_count
        - additional_word_count;

    zero_end_index = FieldMetaTable[metadata_index].word_offset
        + FieldMetaTable[metadata_index].word_count;

    for (data_index = zero_start_index; data_index < zero_end_index; ++data_index) {
        FieldBitData[data_index] = 0;
    }
}


// Shrink (remove) words from a field metadata entry’s bitfield region.
//
// Removes `remove_count` 32-bit words from the metadata entry at `metadata_index`,
// compacting the global FieldBitData array and adjusting subsequent entries’ offsets.
//
// Behavior:
// - If the entry is not the last one, shifts all following data backward to close the gap.
// - Updates word offsets for all later entries to reflect the new layout.
// - Reduces both the per-entry word count and the total global data size.
//
// This is the inverse of field_metadata_grow_word_array().
//
// 0x4E6130
void field_metadata_shrink_word_array(int metadata_index, int remove_count)
{
    int* next_segment_start;

    // If this isn’t the last entry, compact all following data.
    if (metadata_index != FieldMetaCount - 1) {
        next_segment_start = &(FieldBitData[FieldMetaTable[metadata_index + 1].word_offset]);

        // Move later words backward by remove_count.
        memmove(&(next_segment_start[-remove_count]),
            next_segment_start,
            sizeof(*next_segment_start) * (FieldBitDataSize - FieldMetaTable[metadata_index + 1].word_offset));

        // Adjust all later entries’ offsets accordingly.
        AdjustFieldOffsets(metadata_index + 1, FieldMetaCount - 1, -remove_count);
    }

    // Update the current entry’s metadata.
    FieldMetaTable[metadata_index].word_count -= remove_count;
    FieldBitDataSize -= remove_count;
}

// Adjust word offsets for a range of field metadata entries.
//
// Adds `offset_delta` to the `word_offset` of all entries between `start_index`
// and `end_index` inclusive. Used after growing or shrinking a field’s data
// to keep subsequent entries aligned correctly in FieldBitData.
//
// 0x4E61B0
void AdjustFieldOffsets(int start_index, int end_index, int offset_delta)
{
    for (int i = start_index; i <= end_index; i++) {
        FieldMetaTable[i].word_offset += offset_delta;
    }
}

// Convert a bit index into the index of its corresponding 32-bit word.
// For example:
//   bit 0–31   → word index 0
//   bit 32–63  → word index 1
//   bit 64–95  → word index 2
//
// This is used to locate which 32-bit word in the bitfield holds a given bit.
// 0x4E61E0
int convert_bit_index_to_word_index(int bit_index)
{
    return bit_index / 32;
}

// Convert a bit index into its bit mask within the 32-bit word that contains it.
// For example:
//   bit 0  → 0x00000001
//   bit 5  → 0x00000020
//   bit 31 → 0x80000000
//
// This is used to isolate or modify a single bit in a given word.
// 0x4E61F0
int convert_bit_index_to_mask(int bit_index)
{
    return 1 << (bit_index % 32);
}


// 0x4e61f0
// Precompute the population count (number of set bits) for all 16-bit values.
void InitPopCountLookup()
{
    int value; // Current 16-bit value being processed (0..65535).
    uint8_t popCount; // Number of set bits found in 'value'.
    int bitMask; // Bit mask used to test each bit position.
    int bitIdx; // Loop counter for the 16 bit positions.

    // Iterate over every possible 16-bit value.
    for (value = 0; value <= 65535; value++) {
        popCount = 0; // Reset count for this 'value'.
        bitMask = 1; // Start with LSB mask (1 << 0).

        // Test all 16 bits of 'value'.
        for (bitIdx = 16; bitIdx != 0; bitIdx--) {
            // If the current bit is set, increment the count.
            if ((bitMask & value) != 0) {
                popCount++;
            }
            // Move mask to the next bit to the left.
            bitMask *= 2; // Equivalent to: bitMask <<= 1;
        }

        // Store the popcount for this 16-bit value.
        Popcount16Lookup[value] = popCount;
    }
}

// 0x4E6240
void ObjBitMaskTable_Init()
{
    int bit_length;
    int bit_value = 1;
    int lower_mask = 0;

    BitMaskTable[0].lower_mask = 0;
    BitMaskTable[0].upper_mask = 0;

    for (bit_length = 1; bit_length <= 16; bit_length++) {
        lower_mask += bit_value;
        bit_value *= 2;

        BitMaskTable[bit_length].lower_mask = lower_mask;
        BitMaskTable[bit_length].upper_mask = 0;
        BitMaskTable[bit_length + 16].lower_mask = -1;
        BitMaskTable[bit_length + 16].upper_mask = lower_mask;
    }
}
