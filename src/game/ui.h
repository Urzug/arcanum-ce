#ifndef ARCANUM_GAME_UI_H_
#define ARCANUM_GAME_UI_H_

#include "game/context.h"
#include "game/schematic.h"
#include "game/timeevent.h"
#include "game/written.h"

typedef enum UiMessageType {
    UI_MSG_TYPE_LEVEL,
    UI_MSG_TYPE_POISON,
    UI_MSG_TYPE_CURSE,
    UI_MSG_TYPE_BLESS,
    UI_MSG_TYPE_EXCLAMATION,
    UI_MSG_TYPE_QUESTION,
    UI_MSG_TYPE_FEEDBACK,
    UI_MSG_TYPE_SKILL,
    UI_MSG_TYPE_SPELL,
    UI_MSG_TYPE_COLLEGE,
    UI_MSG_TYPE_TECH,
    UI_MSG_TYPE_DEGREE,
    UI_MSG_TYPE_STAT,
    UI_MSG_TYPE_SCHEMATIC,
} UiMessageType;

typedef struct UiMessage {
    int type;
    char* str;
    int field_8;
    int field_C;
    int64_t field_10;
} UiMessage;

static_assert(sizeof(UiMessage) == 0x18, "wrong size");

typedef enum UiPrimaryButton {
    UI_PRIMARY_BUTTON_CHAR,
    UI_PRIMARY_BUTTON_LOGBOOK,
    UI_PRIMARY_BUTTON_TOWNMAP,
    UI_PRIMARY_BUTTON_WORLDMAP,
    UI_PRIMARY_BUTTON_INVENTORY,
    UI_PRIMARY_BUTTON_COUNT,
} UiPrimaryButton;

// TODO: Remove forward declarations.
typedef struct Packet81 Packet81;
typedef struct DialogSerializedData DialogSerializedData;
typedef struct S4F2810 S4F2810;

typedef void(UI_CALLBACKS_FUNC_0)();
typedef void(UI_CALLBACKS_FUNC_4)(int64_t obj);
typedef void(UiUpdateInven)(int64_t obj);
typedef void(UI_CALLBACKS_FUNC_C)();
typedef void(UI_CALLBACKS_FUNC_10)(int64_t a1);
typedef void(UI_CALLBACKS_FUNC_14)(int64_t a1);
typedef void(UI_CALLBACKS_FUNC_18)(int64_t obj);
typedef void(UI_CALLBACKS_FUNC_1C)(int64_t obj, bool a3, int a4);
typedef void(UiShowInvenLoot)(int64_t pc_obj, int64_t target_obj);
typedef void(UiShowInvenIdentify)(int64_t pc_obj, int64_t target_obj);
typedef void(UI_CALLBACKS_FUNC_28)(int64_t obj, int tech, int action);
typedef void(UI_CALLBACKS_FUNC_2C)(int64_t obj, int type);
typedef void(UI_CALLBACKS_FUNC_30)(S4F2810 *a1, int64_t obj, int a3);
typedef void(UiAdjustSkillFunc)(int64_t obj, int skill, int action);
typedef void(UiSetSkillTrainingFunc)(int64_t obj, int skill, int training);
typedef void(UI_CALLBACKS_FUNC_3C)(int64_t a1, int64_t a2);
typedef void(UI_CALLBACKS_FUNC_40)(int a1, int a2);
typedef bool(UI_CALLBACKS_FUNC_44)(int64_t obj);
typedef void(UI_CALLBACKS_FUNC_48)(int64_t obj, int index);
typedef bool(UI_CALLBACKS_FUNC_4C)(int magictech);
typedef void(UI_CALLBACKS_FUNC_50)(int magictech);
typedef void(UiMaintainRefresh)();
typedef void(UI_CALLBACKS_FUNC_58)(int64_t a1, int64_t a2);
typedef void(UiEndDeath)();
typedef void(UiEndGame)();
typedef void(UI_CALLBACKS_FUNC_64)(int a1);
typedef void(UI_CALLBACKS_FUNC_68)(int64_t a1, int64_t a2, int a3);
typedef bool(UI_CALLBACKS_FUNC_6C)(int64_t a1, int a2);
typedef void(UI_CALLBACKS_FUNC_70)();
typedef void(UI_CALLBACKS_FUNC_74)();
typedef void(UI_CALLBACKS_FUNC_78)(int64_t a1, void* a3, int a4);
typedef void(UI_CALLBACKS_FUNC_7C)(char* str);
typedef void(UI_CALLBACKS_FUNC_80)(UiMessage* ui_message);
typedef void(UI_CALLBACKS_FUNC_84)(int a1);
typedef void(UI_CALLBACKS_FUNC_88)();
typedef void(UiWrittenStartObj)(int64_t written_obj, int64_t pc_obj);
typedef void(UiWrittenStartType)(WrittenType written_type, int num);
typedef void(UI_CALLBACKS_FUNC_94)(int64_t a1, int64_t a2);
typedef void(UiFollowerRefresh)();
typedef void(UiFollowerUpdate)();
typedef void(UiTogglePrimaryButton)(UiPrimaryButton btn, bool on);
typedef void(UiSetMapButton)(UiPrimaryButton btn);
typedef void(UI_CALLBACKS_FUNC_A8)(int64_t a1, int64_t a2);
typedef void(UI_CALLBACKS_FUNC_AC)(int a1, char* str);
typedef void(UI_CALLBACKS_FUNC_B0)(int64_t obj);
typedef void(UI_CALLBACKS_FUNC_B4)(int a1, int a2);
typedef void(UiChareditRefresh)();
typedef void(UiProgressBarInit)(int max_value);
typedef void(UiProgressBarUpdate)(int value);
typedef void(UI_CALLBACKS_FUNC_C4)(int64_t a1, int a3);
typedef bool(UI_CALLBACKS_FUNC_C8)(int a1, int64_t a2, int64_t a3);
typedef bool(UI_CALLBACKS_FUNC_CC)(int a1, int64_t a2, int64_t a3);
typedef void(UI_CALLBACKS_FUNC_D0)(int64_t a1, int64_t a2);
typedef bool(UI_CALLBACKS_FUNC_D4)(Packet81* pkt);
typedef void(UI_CALLBACKS_FUNC_D8)(int64_t obj);
typedef void(UI_CALLBACKS_FUNC_DC)();
typedef void(UI_CALLBACKS_FUNC_E0)();
typedef void(UI_CALLBACKS_FUNC_E4)();
typedef void(UI_CALLBACKS_FUNC_E8)(int64_t a1, int64_t a2, int a5, int a6, int a7);
typedef void(UI_CALLBACKS_FUNC_EC)(int64_t a1, int a3);
typedef void(UI_CALLBACKS_FUNC_F0)(int64_t obj, int a2, int a3, int a4, int a5, int a6, const char* str);
typedef void(UI_CALLBACKS_FUNC_F4)(DialogSerializedData* serialized_data, int a2, int a3, int a4, int a5, char* buffer);
typedef void(UI_CALLBACKS_FUNC_F8)(int a1, int a2, void* a3);
typedef bool(UI_CALLBACKS_FUNC_FC)(tig_button_handle_t button_handle);
typedef void(UiRefreshCursor)();
typedef int(UI_CALLBACKS_FUNC_104)(int a1, S4F2810* a2);
typedef void(UI_CALLBACKS_FUNC_108)();
typedef bool(UiWmapRndTimeEventProcess)(TimeEvent* timeevent);
typedef void(UI_CALLBACKS_FUNC_110)(int64_t a1, int64_t a2);
typedef void(UiQueueSlide)(int slide);
typedef bool(UI_CALLBACKS_FUNC_118)();
typedef bool(UI_CALLBACKS_FUNC_11C)();
typedef bool(UI_CALLBACKS_FUNC_120)();
typedef bool(UI_CALLBACKS_FUNC_124)(const char* a1, const char* a2);
typedef void(UI_CALLBACKS_FUNC_128)(int schematic, SchematicInfo* schematic_info);
typedef int64_t(UI_CALLBACKS_FUNC_12C)();
typedef void(UI_CALLBACKS_FUNC_130)(int64_t obj);
typedef bool(UI_CALLBACKS_FUNC_134)(int64_t a1, int64_t a2, int a3);
typedef bool(UI_CALLBACKS_FUNC_138)(int64_t a1, int64_t a2, int a3);
typedef void(UI_CALLBACKS_FUNC_13C)(int64_t a1, int64_t a2, int a3);
typedef bool(UI_CALLBACKS_FUNC_140)(int64_t a1, int64_t a2, int a3);
typedef void(UI_CALLBACKS_FUNC_144)(const char* a1);
typedef void(UI_CALLBACKS_FUNC_148)(const char* a1);
typedef void(UI_CALLBACKS_FUNC_14C)(const char* a1, const char* a2);
typedef void(UI_CALLBACKS_FUNC_150)(const char* a1, const char* a2);
typedef void(UI_CALLBACKS_FUNC_154)(int a1, int a2);
typedef void(UI_CALLBACKS_FUNC_158)(const char* a1, const char* a2);
typedef void(UI_CALLBACKS_FUNC_15C)();
typedef void(UiShowInvenNpcIdentify)(int64_t pc_obj, int64_t target_obj);
typedef void(UiMpChareditCacheTraits)(int player);
typedef void(UiMultiplayerChareditTraitInc)(int player, int trait, int param);
typedef void(UiMultiplayerChareditTraitDec)(int player, int trait, int param);

typedef struct UiCallbacks {
    /* 0000 */ UI_CALLBACKS_FUNC_0* field_0;
    /* 0004 */ UI_CALLBACKS_FUNC_4* field_4;
    /* 0008 */ UiUpdateInven* update_inven;
    /* 000C */ UI_CALLBACKS_FUNC_C* field_C;
    /* 0010 */ UI_CALLBACKS_FUNC_10* field_10;
    /* 0014 */ UI_CALLBACKS_FUNC_14* field_14;
    /* 0018 */ UI_CALLBACKS_FUNC_18* field_18;
    /* 001C */ UI_CALLBACKS_FUNC_1C* field_1C;
    /* 0020 */ UiShowInvenLoot* show_inven_loot;
    /* 0024 */ UiShowInvenIdentify* show_inven_identify;
    /* 0028 */ UI_CALLBACKS_FUNC_28* field_28;
    /* 002C */ UI_CALLBACKS_FUNC_2C* field_2C;
    /* 0030 */ UI_CALLBACKS_FUNC_30* field_30;
    /* 0034 */ UiAdjustSkillFunc* adjust_skill;
    /* 0038 */ UiSetSkillTrainingFunc* set_skill_training;
    /* 003C */ UI_CALLBACKS_FUNC_3C* field_3C;
    /* 0040 */ UI_CALLBACKS_FUNC_40* field_40;
    /* 0044 */ UI_CALLBACKS_FUNC_44* field_44;
    /* 0048 */ UI_CALLBACKS_FUNC_48* field_48;
    /* 004C */ UI_CALLBACKS_FUNC_4C* field_4C;
    /* 0050 */ UI_CALLBACKS_FUNC_50* field_50;
    /* 0054 */ UiMaintainRefresh* maintain_refresh;
    /* 0058 */ UI_CALLBACKS_FUNC_58* field_58;
    /* 005C */ UiEndDeath* end_death;
    /* 0060 */ UiEndGame* end_game;
    /* 0064 */ UI_CALLBACKS_FUNC_64* field_64;
    /* 0068 */ UI_CALLBACKS_FUNC_68* field_68;
    /* 006C */ UI_CALLBACKS_FUNC_6C* field_6C;
    /* 0070 */ UI_CALLBACKS_FUNC_70* field_70;
    /* 0074 */ UI_CALLBACKS_FUNC_74* field_74;
    /* 0078 */ UI_CALLBACKS_FUNC_78* field_78;
    /* 007C */ UI_CALLBACKS_FUNC_7C* field_7C;
    /* 0080 */ UI_CALLBACKS_FUNC_80* field_80;
    /* 0084 */ UI_CALLBACKS_FUNC_84* field_84;
    /* 0088 */ UI_CALLBACKS_FUNC_88* field_88;
    /* 008C */ UiWrittenStartObj* written_start_obj;
    /* 0090 */ UiWrittenStartType* written_start_type;
    /* 0094 */ UI_CALLBACKS_FUNC_94* field_94;
    /* 0098 */ UiFollowerRefresh* follower_refresh;
    /* 009C */ UiFollowerUpdate* follower_update;
    /* 00A0 */ UiTogglePrimaryButton* toggle_primary_button;
    /* 00A4 */ UiSetMapButton* set_map_button;
    /* 00A8 */ UI_CALLBACKS_FUNC_A8* field_A8;
    /* 00AC */ UI_CALLBACKS_FUNC_AC* field_AC;
    /* 00B0 */ UI_CALLBACKS_FUNC_B0* field_B0;
    /* 00B4 */ UI_CALLBACKS_FUNC_B4* field_B4;
    /* 00B8 */ UiChareditRefresh* charedit_refresh;
    /* 00BC */ UiProgressBarInit* progressbar_init;
    /* 00C0 */ UiProgressBarUpdate* progressbar_update;
    /* 00C4 */ UI_CALLBACKS_FUNC_C4* field_C4;
    /* 00C8 */ UI_CALLBACKS_FUNC_C8* field_C8;
    /* 00CC */ UI_CALLBACKS_FUNC_CC* field_CC;
    /* 00D0 */ UI_CALLBACKS_FUNC_D0* field_D0;
    /* 00D4 */ UI_CALLBACKS_FUNC_D4* field_D4;
    /* 00D8 */ UI_CALLBACKS_FUNC_D8* field_D8;
    /* 00DC */ UI_CALLBACKS_FUNC_DC* field_DC;
    /* 00E0 */ UI_CALLBACKS_FUNC_E0* field_E0;
    /* 00E4 */ UI_CALLBACKS_FUNC_E4* field_E4;
    /* 00E8 */ UI_CALLBACKS_FUNC_E8* field_E8;
    /* 00EC */ UI_CALLBACKS_FUNC_EC* field_EC;
    /* 00F0 */ UI_CALLBACKS_FUNC_F0* field_F0;
    /* 00F4 */ UI_CALLBACKS_FUNC_F4* field_F4;
    /* 00F8 */ UI_CALLBACKS_FUNC_F8* field_F8;
    /* 00FC */ UI_CALLBACKS_FUNC_FC* field_FC;
    /* 0100 */ UiRefreshCursor* refresh_cursor;
    /* 0104 */ UI_CALLBACKS_FUNC_104* field_104;
    /* 0108 */ UI_CALLBACKS_FUNC_108* field_108;
    /* 010C */ UiWmapRndTimeEventProcess* wmap_rnd_timeevent_process;
    /* 0110 */ UI_CALLBACKS_FUNC_110* field_110;
    /* 0114 */ UiQueueSlide* queue_slide;
    /* 0118 */ UI_CALLBACKS_FUNC_118* field_118;
    /* 011C */ UI_CALLBACKS_FUNC_11C* field_11C;
    /* 0120 */ UI_CALLBACKS_FUNC_120* field_120;
    /* 0124 */ UI_CALLBACKS_FUNC_124* field_124;
    /* 0128 */ UI_CALLBACKS_FUNC_128* field_128;
    /* 012C */ UI_CALLBACKS_FUNC_12C* field_12C;
    /* 0130 */ UI_CALLBACKS_FUNC_130* field_130;
    /* 0134 */ UI_CALLBACKS_FUNC_134* field_134;
    /* 0138 */ UI_CALLBACKS_FUNC_138* field_138;
    /* 013C */ UI_CALLBACKS_FUNC_13C* field_13C;
    /* 0140 */ UI_CALLBACKS_FUNC_140* field_140;
    /* 0144 */ UI_CALLBACKS_FUNC_144* field_144;
    /* 0148 */ UI_CALLBACKS_FUNC_148* field_148;
    /* 014C */ UI_CALLBACKS_FUNC_14C* field_14C;
    /* 0150 */ UI_CALLBACKS_FUNC_150* field_150;
    /* 0154 */ UI_CALLBACKS_FUNC_154* field_154;
    /* 0158 */ UI_CALLBACKS_FUNC_158* field_158;
    /* 015C */ UI_CALLBACKS_FUNC_15C* field_15C;
    /* 0160 */ UiShowInvenNpcIdentify* show_inven_npc_identify;
    /* 0164 */ UiMpChareditCacheTraits* mp_charedit_cache_traits;
    /* 0168 */ UiMultiplayerChareditTraitInc* mp_charedit_trait_inc;
    /* 016C */ UiMultiplayerChareditTraitDec* mp_charedit_trait_dec;
} UiCallbacks;

void ui_init(UiCallbacks* callbacks);
void ui_exit();
bool ui_timeevent_process(TimeEvent* timeevent);
void sub_4601C0();
void sub_4601D0(int64_t obj);
void ui_update_inven(int64_t obj);
void sub_460240(int64_t a1);
void sub_460260(int64_t a1);
void sub_460280(int64_t obj);
void sub_4602A0(int64_t obj, int a3, int a4);
void ui_show_inven_loot(int64_t pc_obj, int64_t target_obj);
void ui_show_inven_identify(int64_t pc_obj, int64_t target_obj);
void sub_460330(int64_t obj, int tech, int action);
void sub_460360(int64_t obj, int type);
void sub_460380(S4F2810 *a1, int64_t obj, int a3);
void ui_adjust_skill(int64_t obj, int skill, int action);
void ui_set_skill_training(int64_t obj, int skill, int training);
void sub_460410(int64_t a1, int64_t a2);
void sub_460440(int a1, int a2);
int sub_460460(int64_t obj);
int sub_4604A0(int mt_id);
void sub_4604C0(int mt_id);
void ui_maintain_refresh();
void sub_4604F0(int64_t a1, int64_t a2);
void ui_end_death();
void ui_end_game();
void sub_460540(int a1);
void sub_460560(int64_t a1, int64_t a2, int a3);
int sub_460590(int64_t a1, int a2);
void sub_4605C0();
void sub_4605D0();
void sub_4605E0(int64_t a1, void* a2, int a3);
void sub_460610(char* str);
void sub_460630(UiMessage* ui_message);
void sub_460650(UiMessage* ui_message, unsigned int milliseconds);
void sub_4606C0(int a1);
void sub_4606E0();
void ui_written_start_obj(int64_t written_obj, int64_t pc_obj);
void ui_written_start_type(WrittenType written_type, int num);
void sub_460740(int64_t a1, int64_t a2);
void ui_follower_refresh();
void ui_follower_update();
void ui_toggle_primary_button(UiPrimaryButton btn, bool on);
void ui_set_map_button(UiPrimaryButton btn);
void sub_4607D0(int64_t a1, int64_t a2);
void sub_460800(int a1, char* str);
void sub_460820(int64_t obj);
void sub_460840(int a1, int a2);
void ui_charedit_refresh();
void ui_progressbar_init(int max_value);
void ui_progressbar_update(int value);
void sub_4608B0(int64_t a1, int a2);
void sub_4608D0(int a1, int64_t a2, int64_t a3);
void sub_460900(int a1, int64_t a2, int64_t a3);
void sub_460930(int64_t a1, int64_t a2);
void sub_460960(Packet81* pkt);
void sub_460980(int64_t a1);
void sub_4609E0(int64_t a1, int64_t a2, int a5, int a6, int a7);
void sub_460A20(int64_t a1, int a3);
void sub_460A40(int64_t obj, int a3, int a4, int a5, int a6, int a7, const char* str);
void sub_460A80(DialogSerializedData* a1, int a2, int a3, int a4, int a5, char* buffer);
void sub_460AB0(int a1, int a2, void* a3);
void sub_460AD0(int a1);
void ui_refresh_cursor();
int sub_460B00(int a1, S4F2810* a2);
void sub_460B20();
bool ui_wmap_rnd_timeevent_process(TimeEvent* timeevent);
void sub_460B50(int64_t a1, int64_t a2);
void ui_queue_slide(int slide);
int sub_460BA0();
int sub_460BB0();
int sub_460BC0();
int sub_460BE0(const char* a1, const char* a2);
void sub_460C00(int schematic, SchematicInfo* schematic_info);
int64_t sub_460C20();
void sub_460C30(int64_t a1);
int sub_460C50(int64_t a1, int64_t a2, int a3);
int sub_460C80(int64_t a1, int64_t a2, int a3);
void sub_460CB0(int64_t a1, int64_t a2, int a3);
int sub_460CE0(int64_t a1, int64_t a2, int a3);
void sub_460D10(const char* a1);
void sub_460D30(const char* a1);
void sub_460D50(const char* a1, const char* a2);
void sub_460D70(const char* a1, const char* a2);
void sub_460D90(int a1, int a2);
void sub_460DB0(const char* a1, const char* a2);
void sub_460DD0();
void ui_show_inven_npc_identify(int64_t pc_obj, int64_t target_obj);
void ui_mp_charedit_cache_traits(int player);
void ui_mp_charedit_trait_inc(int player, int trait, int param);
void ui_mp_charedit_trait_dec(int player, int trait, int param);

#endif /* ARCANUM_GAME_UI_H_ */
