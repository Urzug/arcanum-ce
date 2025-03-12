#ifndef ARCANUM_GAME_DIALOG_H_
#define ARCANUM_GAME_DIALOG_H_

#include "game/context.h"
#include "game/obj.h"
#include "game/object.h"

typedef struct DialogState {
    /* 0000 */ int dlg;
    /* 0008 */ int64_t pc_obj;
    /* 0010 */ Ryan field_10;
    /* 0038 */ int64_t npc_obj;
    /* 0040 */ Ryan field_40;
    /* 0068 */ int num;
    /* 006C */ int script_num;
    /* 0070 */ char reply[1000];
    /* 0458 */ int speech_id;
    /* 045C */ int num_options;
    /* 0460 */ char options[5][1000];
    /* 17E8 */ int field_17E8;
    /* 17EC */ int field_17EC;
    /* 17F0 */ int field_17F0[5];
    /* 1804 */ int field_1804[5];
    /* 1818 */ int field_1818[5];
    /* 182C */ char* actions[5];
    /* 1840 */ int field_1840;
    /* 1844 */ int seed;
} DialogState;

static_assert(sizeof(DialogState) == 0x1848, "wrong size");

typedef struct DialogSerializedData {
    /* 0000 */ int field_0;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID field_8;
    /* 0020 */ ObjectID field_20;
    /* 0038 */ int field_38;
    /* 003C */ int field_3C;
    /* 0040 */ int field_40;
    /* 0044 */ int field_44;
    /* 0048 */ int field_48;
    /* 004C */ int field_4C;
    /* 0050 */ int field_50[5];
    /* 0064 */ int field_64[5];
    /* 0078 */ int field_78;
    /* 007C */ int field_7C;
    /* 0080 */ int field_80[5];
    /* 0094 */ int field_94[5];
    /* 00A8 */ int field_A8[5];
    /* 00BC */ int field_BC;
    /* 00C0 */ int field_C0;
} DialogSerializedData;

// TODO: Figure out size.
// static_assert(sizeof(DialogSerializedData) == 0xC4, "wrong size");

bool dialog_init(GameInitInfo* init_info);
void dialog_exit();
bool dialog_load(const char* path, int* dlg_ptr);
void dialog_unload(int dlg);
void sub_412F60(int dlg);
bool sub_412FD0(DialogState* a1);
void sub_413130(DialogState* a1, int a2);
void sub_413280(DialogState* a1);
void sub_4132A0(int64_t a1, int64_t a2, char* buffer);
void sub_413360(int64_t a1, int64_t a2, DialogState* a3);
void sub_4133B0(int64_t a1, int64_t a2, char* buffer, int* a4);
void dialog_copy_npc_sell_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_wont_sell_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_normally_wont_sell_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_buy_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_wont_buy_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_wont_buy_stolen_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_normally_wont_buy_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_buy_for_less_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_not_enough_money_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void dialog_copy_npc_let_me_handle_msg(int64_t npc_obj, int64_t pc_obj, char* buffer);
void sub_413A30(DialogState* a1, bool a2);
void dialog_copy_npc_wont_follow_msg(int64_t npc_obj, int64_t pc_obj, int reason, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_order_ok_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_order_no_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_insult_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_accidental_attack_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_fleeing_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_witness_pc_critical_msg(int64_t npc_obj, int64_t pc_obj, int type, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_gamble_msg(int64_t npc_obj, int64_t pc_obj, int type, char* buffer);
void dialog_copy_npc_warning_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_newspaper_msg(int64_t npc_obj, int64_t pc_obj, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_warning_follow_msg(int64_t npc_obj, int64_t pc_obj, int reason, char* buffer, int* speech_id_ptr);
void dialog_copy_npc_upset_attacking_msg(int64_t npc_obj, int64_t pc_obj, int reason, char* buffer, int* speech_id_ptr);
void sub_414490(int64_t a1, int64_t a2, char* a3, int* a4);
void sub_414540(int64_t a1, int64_t a2, char* a3);
void sub_4145D0(int64_t a1, int64_t a2, char* a3);
void sub_414660(int64_t a1, int64_t a2, char* a3);
void sub_4146F0(int64_t a1, int64_t a2, char* a3);
void sub_414780(int64_t a1, int64_t a2, char* a3);
void dialog_check();
void dialog_enable_numbers();
void sub_418A00(int a1, int* a2, int* a3);

#endif /* ARCANUM_GAME_DIALOG_H_ */
