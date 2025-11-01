#ifndef ARCANUM_GAME_OBJ_PRIVATE_H_
#define ARCANUM_GAME_OBJ_PRIVATE_H_

#include "game/obj.h"
#include "game/obj_id.h"
#include "game/obj_pool.h"
#include "game/quest.h"
#include "game/sa.h"
#include "game/script.h"

typedef enum SaType {
    SA_TYPE_INVALID = 0,
    SA_TYPE_BEGIN = 1,
    SA_TYPE_END = 2,
    SA_TYPE_INT32 = 3,
    SA_TYPE_INT64 = 4,
    SA_TYPE_INT32_ARRAY = 5,
    SA_TYPE_INT64_ARRAY = 6,
    SA_TYPE_UINT32_ARRAY = 7,
    SA_TYPE_UINT64_ARRAY = 8,
    SA_TYPE_SCRIPT = 9,
    SA_TYPE_QUEST = 10,
    SA_TYPE_STRING = 11,
    SA_TYPE_HANDLE = 12,
    SA_TYPE_HANDLE_ARRAY = 13,
    SA_TYPE_PTR = 14,
    SA_TYPE_PTR_ARRAY = 15,
} SaType;

// Structure representing a dynamically growing memory write buffer.
// Used to write sequential data into memory without knowing final size upfront.
typedef struct MemoryWriteBuffer {
    uint8_t* base_pointer; /**< Pointer to the start of the allocated buffer. */
    uint8_t* write_pointer; /**< Current position to write the next byte. */
    int total_capacity; /**< Total number of bytes currently allocated. */
    int remaining_capacity; /**< Number of bytes left before reallocation is required. */
} MemoryWriteBuffer;

typedef struct ObjSa {
    /* 0000 */ int type;
    /* 0004 */ void* ptr;
    /* 0008 */ int idx;
    /* 000C */ int field_C;
    /* 0010 */ union {
        int value;
        int64_t value64;
        char* str;
        ObjectID oid;
        Script scr;
        PcQuestState quest;
        intptr_t ptr;
    } storage;
} ObjSa;

void ObjPrivate_Enable();
void ObjPrivate_Disable();
void object_field_deallocate(ObjSa* a1);
void object_field_apply_from_storage(ObjSa* a1);
void object_field_load_into_storage(ObjSa* a1);
void object_field_deep_copy_value(ObjSa* a1, void* value);
bool object_field_read_from_file(ObjSa* a1, TigFile* stream);
bool object_field_read_from_file_no_dealloc(ObjSa* a1, TigFile* stream);
void object_field_read_from_memory(ObjSa* a1, uint8_t** data);
bool object_field_write_to_file(ObjSa* a1, TigFile* stream);
void sub_4E4990(ObjSa* a1, MemoryWriteBuffer* a2);
void sub_4E4B70(ObjSa* a1);
int sub_4E4BA0(ObjSa* a1);
void sub_4E4BD0(MemoryWriteBuffer* a1);
void sub_4E4C00(const void* data, int size, MemoryWriteBuffer* a3);
void sub_4E4C50(void* buffer, int size, uint8_t** data);
void sub_4E59B0();
void sub_4E5A50();
int field_metadata_acquire();
int field_metadata_release(int a1);
int field_metadata_clone(int a1);
void field_metadata_set_or_clear_bit(int a1, int a2, bool a3);
int field_metadata_test_bit(int a1, int a2);
int field_metadata_count_set_bits_up_to(int a1, int a2);
bool field_metadata_iterate_set_bits(int a1, bool (*callback)(int));
bool field_metadata_serialize_to_tig_file(int a1, TigFile* stream);
bool field_metadata_deserialize_from_tig_file(int* a1, TigFile* stream);
int field_metadata_calculate_export_size(int a1);
void field_metadata_export_to_memory(int a1, void* a2);
void field_metadata_import_from_memory(int* a1, uint8_t** data);
int count_set_bits_in_word_up_to_limit(int a1, int a2);

#endif /* ARCANUM_GAME_OBJ_PRIVATE_H_ */
