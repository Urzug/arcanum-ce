#include "game/obj_private.h"

#include <stdio.h>

#include "game/map.h"
#include "game/obj_file.h"
#include "game/object.h"
#include "game/sector.h"

typedef struct ObjFieldMeta {
    /* 0000 */ int field_0;
    /* 0004 */ int field_4;
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

// 0x4E4180
void sub_4E4180(ObjSa* a1)
{
    switch (a1->type) {
    case SA_TYPE_INT32:
        a1->storage.value = *(int*)a1->ptr;
        break;
    case SA_TYPE_INT64:
        if (*(int64_t**)a1->ptr != NULL) {
            a1->storage.value64 = **(int64_t**)a1->ptr;
        } else {
            a1->storage.value64 = 0;
        }
        break;
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_get((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        } else {
            a1->storage.value = 0;
        }
        break;
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_get((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        } else {
            a1->storage.value64 = 0;
        }
        break;
    case SA_TYPE_SCRIPT:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_get((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        } else {
            memset(&(a1->storage.scr), 0, sizeof(a1->storage.scr));
        }
        break;
    case SA_TYPE_QUEST:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_get((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        } else {
            memset(&(a1->storage.quest), 0, sizeof(a1->storage.quest));
        }
        break;
    case SA_TYPE_STRING:
        if (*(char**)a1->ptr != NULL) {
            a1->storage.str = STRDUP(*(char**)a1->ptr);
        } else {
            a1->storage.str = NULL;
        }
        break;
    case SA_TYPE_HANDLE:
        if (*(ObjectID**)a1->ptr != NULL) {
            a1->storage.oid = **(ObjectID**)a1->ptr;
        } else {
            a1->storage.oid.type = OID_TYPE_NULL;
        }
        break;
    case SA_TYPE_HANDLE_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_get((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        } else {
            a1->storage.oid.type = OID_TYPE_NULL;
        }
        break;
    case SA_TYPE_PTR:
        a1->storage.ptr = *(intptr_t*)a1->ptr;
        break;
    case SA_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sa_get((SizeableArray**)a1->ptr, a1->idx, &(a1->storage));
        } else {
            a1->storage.ptr = 0;
        }
        break;
    }
}

// 0x4E4280
void sub_4E4280(ObjSa* a1, void* value)
{
    switch (a1->type) {
    case SA_TYPE_INT32:
        *(int*)value = *(int*)a1->ptr;
        break;
    case SA_TYPE_INT64:
        if (*(int64_t**)a1->ptr != NULL) {
            *(int64_t**)value = (int64_t*)MALLOC(sizeof(int64_t));
            **(int64_t**)value = **(int64_t**)a1->ptr;
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
        if (*(SizeableArray**)a1->ptr != NULL) {
            sub_4E74A0((SizeableArray**)value, (SizeableArray**)a1->ptr);
        } else {
            *(SizeableArray**)value = NULL;
        }
        break;
    case SA_TYPE_STRING:
        if (*(char**)a1->ptr != NULL) {
            *(char**)value = STRDUP(*(char**)a1->ptr);
        } else {
            *(char**)value = NULL;
        }
        break;
    case SA_TYPE_HANDLE:
        if (*(ObjectID**)a1->ptr != NULL) {
            *(ObjectID**)value = (ObjectID*)MALLOC(sizeof(ObjectID));
            **(ObjectID**)value = **(ObjectID**)a1->ptr;
        } else {
            *(ObjectID**)value = NULL;
        }
        break;
    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        assert(0);
    }
}

// 0x4E4360
bool sub_4E4360(ObjSa* a1, TigFile* stream)
{
    uint8_t presence;
    int size;

    switch (a1->type) {
    case SA_TYPE_INT32:
        if (!objf_read(a1->ptr, sizeof(int), stream)) {
            return false;
        }
        return true;
    case SA_TYPE_INT64:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            if (*(int64_t**)a1->ptr != NULL) {
                FREE(*(int64_t**)a1->ptr);
                *(int64_t**)a1->ptr = NULL;
            }
            return true;
        }
        if (*(int64_t**)a1->ptr == NULL) {
            *(int64_t**)a1->ptr = (int64_t*)MALLOC(sizeof(int64_t));
        }
        if (!objf_read(*(int64_t**)a1->ptr, sizeof(int64_t), stream)) {
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
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            *(SizeableArray**)a1->ptr = NULL;
            return true;
        }
        if (!sa_read((SizeableArray**)a1->ptr, stream)) {
            return false;
        }
        return true;
    case SA_TYPE_STRING:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (*(char**)a1->ptr != NULL) {
            FREE(*(char**)a1->ptr);
        }
        if (!presence) {
            *(char**)a1->ptr = NULL;
            return true;
        }
        if (!objf_read(&size, sizeof(size), stream)) {
            return false;
        }
        *(char**)a1->ptr = (char*)MALLOC(size + 1);
        if (!objf_read(*(char**)a1->ptr, size + 1, stream)) {
            return false;
        }
        return true;
    case SA_TYPE_HANDLE:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            if (*(ObjectID**)a1->ptr != NULL) {
                FREE(*(ObjectID**)a1->ptr);
                *(ObjectID**)a1->ptr = NULL;
            }
            return true;
        }
        if (*(ObjectID**)a1->ptr == NULL) {
            *(ObjectID**)a1->ptr = (ObjectID*)MALLOC(sizeof(ObjectID));
        }
        if (!objf_read(*(ObjectID**)a1->ptr, sizeof(ObjectID), stream)) {
            return false;
        }
        return true;
    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        assert(0);
    default:
        return false;
    }
}

// 0x4E44F0
bool sub_4E44F0(ObjSa* a1, TigFile* stream)
{
    uint8_t presence;
    int size;

    switch (a1->type) {
    case SA_TYPE_INT32:
        if (!objf_read(a1->ptr, sizeof(int), stream)) {
            return false;
        }
        return true;
    case SA_TYPE_INT64:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            *(int64_t**)a1->ptr = NULL;
            return true;
        }
        *(int64_t**)a1->ptr = (int64_t*)MALLOC(sizeof(int64_t));
        if (!objf_read(*(int64_t**)a1->ptr, sizeof(int64_t), stream)) {
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
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            *(SizeableArray**)a1->ptr = NULL;
            return true;
        }
        if (!sa_read_no_dealloc((SizeableArray**)a1->ptr, stream)) {
            return false;
        }
        return true;
    case SA_TYPE_STRING:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            *(char**)a1->ptr = NULL;
            return true;
        }
        if (!objf_read(&size, sizeof(size), stream)) {
            return false;
        }
        *(char**)a1->ptr = (char*)MALLOC(size + 1);
        if (!objf_read(*(char**)a1->ptr, size + 1, stream)) {
            return false;
        }
        return true;
    case SA_TYPE_HANDLE:
        if (!objf_read(&presence, sizeof(presence), stream)) {
            return false;
        }
        if (!presence) {
            *(ObjectID**)a1->ptr = NULL;
            return true;
        }
        *(ObjectID**)a1->ptr = (ObjectID*)MALLOC(sizeof(ObjectID));
        if (!objf_read(*(ObjectID**)a1->ptr, sizeof(ObjectID), stream)) {
            return false;
        }
        return true;
    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        assert(0);
    default:
        return false;
    }
}

// 0x4E4660
void sub_4E4660(ObjSa* a1, uint8_t** data)
{
    uint8_t presence;
    int size;

    switch (a1->type) {
    case SA_TYPE_INT32:
        sub_4E4C50(a1->ptr, sizeof(int), data);
        return;
    case SA_TYPE_INT64:
        sub_4E4C50(&presence, sizeof(presence), data);
        if (!presence) {
            if (*(int64_t**)a1->ptr != NULL) {
                FREE(*(int64_t**)a1->ptr);
                *(int64_t**)a1->ptr = NULL;
            }
            return;
        }
        if (*(int64_t**)a1->ptr == NULL) {
            *(int64_t**)a1->ptr = (int64_t*)MALLOC(sizeof(int64_t));
        }
        sub_4E4C50(*(int64_t**)a1->ptr, sizeof(int64_t), data);
        return;
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        sub_4E4C50(&presence, sizeof(presence), data);
        if (!presence) {
            if (*(SizeableArray**)a1->ptr != NULL) {
                FREE(*(SizeableArray**)a1->ptr);
                *(SizeableArray**)a1->ptr = NULL;
            }
            return;
        }
        sub_4E7820((SizeableArray**)a1->ptr, data);
        return;
    case SA_TYPE_STRING:
        sub_4E4C50(&presence, sizeof(presence), data);
        if (*(char**)a1->ptr != NULL) {
            FREE(*(char**)a1->ptr);
        }
        if (!presence) {
            *(char**)a1->ptr = NULL;
            return;
        }
        sub_4E4C50(&size, sizeof(size), data);
        *(char**)a1->ptr = (char*)MALLOC(size + 1);
        sub_4E4C50(*(char**)a1->ptr, size + 1, data);
        return;
    case SA_TYPE_HANDLE:
        sub_4E4C50(&presence, sizeof(presence), data);
        if (!presence) {
            if (*(ObjectID**)a1->ptr != NULL) {
                FREE(*(ObjectID**)a1->ptr);
                *(ObjectID**)a1->ptr = NULL;
            }
            return;
        }
        if (*(ObjectID**)a1->ptr == NULL) {
            *(ObjectID**)a1->ptr = (ObjectID*)MALLOC(sizeof(ObjectID));
        }
        sub_4E4C50(*(ObjectID**)a1->ptr, sizeof(ObjectID), data);
        return;
    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        assert(0);
    }
}

// 0x4E47E0
bool sub_4E47E0(ObjSa* a1, TigFile* stream)
{
    uint8_t presence;
    int size;

    switch (a1->type) {
    case SA_TYPE_INT32:
        if (!objf_write((int*)a1->ptr, sizeof(int), stream)) return false;
        return true;
    case SA_TYPE_INT64:
        if (*(int64_t**)a1->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
            if (!objf_write(*(int64_t**)a1->ptr, sizeof(int64_t), stream)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
        }
        return true;
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
            if (!sa_write((SizeableArray**)a1->ptr, stream)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
        }
        return true;
    case SA_TYPE_STRING:
        if (*(char**)a1->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
            size = (int)strlen(*(char**)a1->ptr);
            if (!objf_write(&size, sizeof(size), stream)) return false;
            if (!objf_write(*(char**)a1->ptr, size + 1, stream)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
        }
        return true;
    case SA_TYPE_HANDLE:
        if (*(ObjectID**)a1->ptr != NULL) {
            presence = 1;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
            if (!objf_write(*(ObjectID**)a1->ptr, sizeof(ObjectID), stream)) return false;
        } else {
            presence = 0;
            if (!objf_write(&presence, sizeof(presence), stream)) return false;
        }
        return true;
    case SA_TYPE_PTR:
    case SA_TYPE_PTR_ARRAY:
        assert(0);
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

// 0x4E4B70
void sub_4E4B70(ObjSa* a1)
{
    switch (a1->type) {
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
    case SA_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            sub_4E7760((SizeableArray**)a1->ptr, a1->idx);
        }
        break;
    }
}

// 0x4E4BA0
int sub_4E4BA0(ObjSa* a1)
{
    switch (a1->type) {
    case SA_TYPE_INT32_ARRAY:
    case SA_TYPE_INT64_ARRAY:
    case SA_TYPE_UINT32_ARRAY:
    case SA_TYPE_UINT64_ARRAY:
    case SA_TYPE_SCRIPT:
    case SA_TYPE_QUEST:
    case SA_TYPE_HANDLE_ARRAY:
    case SA_TYPE_PTR_ARRAY:
        if (*(SizeableArray**)a1->ptr != NULL) {
            return sa_count((SizeableArray**)a1->ptr);
        }
        break;
    }

    return 0;
}

// 0x4E4BD0
void sub_4E4BD0(MemoryWriteBuffer* buffer)
{
    buffer->base_pointer = (uint8_t*)MALLOC(256);
    buffer->write_pointer = buffer->base_pointer;
    buffer->total_capacity = 256;
    buffer->remaining_capacity = buffer->total_capacity;
}

// 0x4E4C00
void sub_4E4C00(const void* data, int size, MemoryWriteBuffer* buffer)
{
    ensure_memory_capacity(buffer, size);
    memcpy(buffer->write_pointer, data, size);
    buffer->write_pointer += size;
    buffer->remaining_capacity -= size;
}

// 0x4E4C50
void sub_4E4C50(void* buffer, int size, uint8_t** data)
{
    memcpy(buffer, *data, size);
    (*data) += size;
}

// 0x4E4C80
void ensure_memory_capacity(MemoryWriteBuffer* a1, int size)
{
    int extra_size;
    int new_size;

    extra_size = size - a1->remaining_capacity;
    if (extra_size > 0) {
        new_size = (extra_size / 256 + 1) * 256;
        a1->total_capacity += new_size;
        a1->base_pointer = (uint8_t*)REALLOC(a1->base_pointer, a1->total_capacity);
        a1->write_pointer = a1->base_pointer + a1->total_capacity - a1->remaining_capacity - new_size;
        a1->remaining_capacity += new_size;
    }
}

// 0x4E59B0
void sub_4E59B0()
{
    FieldMetaCount = 0;
    FieldMetaCapacity = 4096;
    FreeIndexCount = 0;
    FreeIndexCapacity = 4096;
    FieldBitDataSize = 0;
    FieldBitDataCapacity = 8192;
    FieldMetaTable = (ObjFieldMeta*)MALLOC(sizeof(*FieldMetaTable) * FieldMetaCapacity);
    FreeFieldMetaIndices = (int*)MALLOC(sizeof(*FreeFieldMetaIndices) * FreeIndexCapacity);
    FieldBitData = (int*)MALLOC(sizeof(*FieldBitData) * FieldBitDataCapacity);
    Popcount16Lookup = (uint8_t*)MALLOC(65536);
    BitMaskTable = (BitMaskPair*)MALLOC(sizeof(*BitMaskTable) * 33);
    InitPopCountLookup();
    ObjBitMaskTable_Init();
    ObjPrivateStorageAllocated = true;
}

// 0x4E5A50
void sub_4E5A50()
{
    FREE(BitMaskTable);
    FREE(Popcount16Lookup);
    FREE(FieldBitData);
    FREE(FreeFieldMetaIndices);
    FREE(FieldMetaTable);
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
        FieldMetaTable[0].field_4 = index;
    } else {
        FieldMetaTable[index].field_4 = FieldMetaTable[index - 1].field_0 + FieldMetaTable[index - 1].field_4;
    }

    FieldMetaTable[index].field_0 = 0;
    field_metadata_grow_word_array(index, 2);

    return index;
}

// 0x4E5B40
int field_metadata_release(int a1)
{
    ObjFieldMeta* v1;
    int index;

    v1 = &(FieldMetaTable[a1]);
    if (v1->field_0 != 2) {
        field_metadata_shrink_word_array(a1, v1->field_0 - 2);
    }

    for (index = v1->field_4; index < v1->field_0 + v1->field_4; index++) {
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
    v2 = FieldMetaTable[a1].field_0 - FieldMetaTable[v1].field_0;
    if (v2 != 0) {
        field_metadata_grow_word_array(v1, v2);
    }

    for (index = 0; index < FieldMetaTable[a1].field_0; index++) {
        FieldBitData[FieldMetaTable[v1].field_4 + index] = FieldBitData[FieldMetaTable[a1].field_4 + index];
    }

    return v1;
}

// 0x4E5C60
void field_metadata_set_or_clear_bit(int a1, int a2, bool a3)
{
    int v1;
    int v2;

    v1 = convert_bit_index_to_word_index(a2);
    if (v1 >= FieldMetaTable[a1].field_0) {
        if (!a3) {
            return;
        }

        field_metadata_grow_word_array(a1, v1 - FieldMetaTable[a1].field_0 + 1);
    }

    v2 = convert_bit_index_to_mask(a2);
    if (a3) {
        FieldBitData[v1 + FieldMetaTable[a1].field_4] |= v2;
    } else {
        FieldBitData[v1 + FieldMetaTable[a1].field_4] &= ~v2;
    }
}

// 0x4E5CE0
int field_metadata_test_bit(int a1, int a2)
{
    int v1;
    int v2;

    v1 = convert_bit_index_to_word_index(a2);
    if (v1 > FieldMetaTable[a1].field_0 - 1) {
        return 0;
    }

    v2 = convert_bit_index_to_mask(a2);
    return v2 & FieldBitData[v1 + FieldMetaTable[a1].field_4];
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
    int base_word_index = FieldMetaTable[metadata_index].field_4;

    // Word index that contains the (exclusive) upper-bound bit.
    int limit_word_index = convert_bit_index_to_word_index(bit_index_limit);

    // Absolute index into the global array of the limit word for this entry.
    int stop_word_index = base_word_index + limit_word_index;

    // If the limit word exists, count only the lower (bit_index_limit % 32) bits of it.
    // This yields exclusive semantics: we do not include the bit at bit_index_limit.
    if (limit_word_index < FieldMetaTable[metadata_index].field_0) {
        total_count += count_set_bits_in_word_up_to_limit(
            FieldBitData[stop_word_index],
            bit_index_limit % 32 // number of lower bits to include (0..31)
        );
    } else {
        // If the limit is beyond the current words, cap at the end of this entry's region.
        stop_word_index = base_word_index + FieldMetaTable[metadata_index].field_0;
    }

    // Count all full words strictly before the limit word.
    while (base_word_index < stop_word_index) {
        total_count += count_set_bits_in_word_up_to_limit(FieldBitData[base_word_index++], 32);
    }

    return total_count;
}


// 0x4E5DB0
bool field_metadata_iterate_set_bits(int a1, bool (*callback)(int))
{
    int v1;
    int v2;
    int pos;
    int idx;
    int bit;
    unsigned int flags;

    v1 = FieldMetaTable[a1].field_4;
    v2 = FieldMetaTable[a1].field_0 + v1;

    pos = 0;
    for (idx = v1; idx < v2; idx++) {
        flags = 1;
        for (bit = 0; bit < 32; bit++) {
            if ((flags & FieldBitData[idx]) != 0) {
                if (!callback(pos)) {
                    return false;
                }
            }
            flags *= 2;
            pos++;
        }
    }

    return true;
}

// 0x4E5E20
bool field_metadata_serialize_to_tig_file(int a1, TigFile* stream)
{
    if (tig_file_fwrite(&(FieldMetaTable[a1].field_0), sizeof(int), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fwrite(&(FieldBitData[FieldMetaTable[a1].field_4]), sizeof(int) * FieldMetaTable[a1].field_0, 1, stream) != 1) {
        return false;
    }

    return true;
}

// 0x4E5E80
bool field_metadata_deserialize_from_tig_file(int* a1, TigFile* stream)
{
    int v1;
    int v2;

    v1 = field_metadata_acquire();

    if (tig_file_fread(&v2, sizeof(v2), 1, stream) != 1) {
        return false;
    }

    if (v2 != FieldMetaTable[v1].field_0 && v2 - FieldMetaTable[v1].field_0 > 0) {
        field_metadata_grow_word_array(v1, v2 - FieldMetaTable[v1].field_0);
    }

    if (tig_file_fread(&(FieldBitData[FieldMetaTable[v1].field_4]), 4 * v2, 1, stream) != 1) {
        return false;
    }

    *a1 = v1;

    return true;
}

// 0x4E5F10
int field_metadata_calculate_export_size(int a1)
{
    return 4 * (FieldMetaTable[a1].field_0 + 1);
}

// 0x4E5F30
void field_metadata_export_to_memory(int a1, void* a2)
{
    int v1;
    int v2;

    v1 = FieldMetaTable[a1].field_4;
    v2 = FieldMetaTable[a1].field_0;

    *(int*)a2 = FieldMetaTable[a1].field_0;
    memcpy((int*)a2 + 1, &(FieldBitData[v1]), sizeof(int) * v2);
}

// 0x4E5F70
void field_metadata_import_from_memory(int* a1, uint8_t** data)
{
    int v1;
    int v2;

    v1 = field_metadata_acquire();

    sub_4E4C50(&v2, sizeof(v2), data);

    if (v2 != FieldMetaTable[v1].field_0) {
        field_metadata_grow_word_array(v1, v2 - FieldMetaTable[v1].field_0);
    }

    sub_4E4C50(&(FieldBitData[FieldMetaTable[v1].field_4]), 4 * v2, data);

    *a1 = v1;
}

// 0x4E5FE0
int count_set_bits_in_word_up_to_limit(int a1, int a2)
{
    return Popcount16Lookup[BitMaskTable[a2].lower_mask & (a1 & 0xFFFF)]
        + Popcount16Lookup[BitMaskTable[a2].upper_mask & ((a1 >> 16) & 0xFFFF)];
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
        next_segment_data = &(FieldBitData[FieldMetaTable[metadata_index + 1].field_4]);

        // Number of words occupied by all segments after the next segment's start.
        words_to_move = (size_t)(FieldBitDataSize - FieldMetaTable[metadata_index + 1].field_4);

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
    FieldMetaTable[metadata_index].field_0 += additional_word_count;
    FieldBitDataSize += additional_word_count;

    // Zero-initialize the newly inserted words for this entry.
    zero_start_index = FieldMetaTable[metadata_index].field_4
        + FieldMetaTable[metadata_index].field_0
        - additional_word_count;

    zero_end_index = FieldMetaTable[metadata_index].field_4
        + FieldMetaTable[metadata_index].field_0;

    for (data_index = zero_start_index; data_index < zero_end_index; ++data_index) {
        FieldBitData[data_index] = 0;
    }
}


// 0x4E6130
void field_metadata_shrink_word_array(int a1, int a2)
{
    int* v1;

    if (a1 != FieldMetaCount - 1) {
        v1 = &(FieldBitData[FieldMetaTable[a1 + 1].field_4]);
        memmove(&(v1[-a2]), v1, sizeof(*v1) * (FieldBitDataSize - FieldMetaTable[a1 + 1].field_4));
        AdjustFieldOffsets(a1 + 1, FieldMetaCount - 1, -a2);
    }

    FieldMetaTable[a1].field_0 -= a2;
    FieldBitDataSize -= a2;
}

// 0x4E61B0
void AdjustFieldOffsets(int start, int end, int inc)
{
    int index;

    for (index = start; index <= end; index++) {
        FieldMetaTable[index].field_4 += inc;
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
