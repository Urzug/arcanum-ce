#ifndef ARCANUM_GAME_GAMELIB_H_
#define ARCANUM_GAME_GAMELIB_H_

#include "game/context.h"
#include "game/settings.h"
#include "game/timeevent.h"

typedef bool(GameExtraSaveFunc)();
typedef bool(GameExtraLoadFunc)();

typedef enum GameDifficulty {
    GAME_DIFFICULTY_EASY,
    GAME_DIFFICULTY_NORMAL,
    GAME_DIFFICULTY_HARD,
} GameDifficulty;

typedef struct GameModuleList {
    unsigned int count;
    unsigned int selected;
    char** paths;
} GameModuleList;

typedef struct GameSaveList {
    char* module;
    unsigned int count;
    char** paths;
} GameSaveList;

typedef enum GameSaveListOrder {
    GAME_SAVE_LIST_ORDER_DATE,
    GAME_SAVE_LIST_ORDER_NAME,
} GameSaveListOrder;

typedef struct GameSaveInfo {
    /* 0000 */ int version;
    /* 0004 */ char name[256];
    /* 0104 */ char module_name[256];
    /* 0204 */ int pc_portrait;
    /* 0208 */ int pc_level;
    /* 020C */ int field_20C;
    /* 0210 */ int64_t pc_location;
    /* 0218 */ char description[256];
    /* 0318 */ TigVideoBuffer* thumbnail_video_buffer;
    /* 031C */ int field_31C;
    /* 0320 */ DateTime datetime;
    /* 0328 */ char pc_name[24];
    /* 0340 */ int field_340;
    /* 0344 */ int field_344;
    /* 0348 */ int field_348;
    /* 034C */ int field_34C;
    /* 0350 */ int field_350;
    /* 0354 */ int field_354;
    /* 0358 */ int field_358;
    /* 035C */ int story_state;
} GameSaveInfo;

static_assert(sizeof(GameSaveInfo) == 0x360, "wrong size");

extern unsigned int gamelib_ping_time;
extern Settings settings;
extern TigVideoBuffer* gamelib_scratch_video_buffer;

bool gamelib_init(GameInitInfo* init_info);
void gamelib_reset();
void gamelib_exit();
void gamelib_ping();
void gamelib_resize(GameResizeInfo* resize_info);
void gamelib_default_module_name_set(const char* name);
const char* gamelib_default_module_name_get();
void gamelib_modlist_create(GameModuleList* module_list, int type);
void gamelib_modlist_destroy(GameModuleList* module_list);
bool gamelib_mod_load(const char* path);
bool gamelib_mod_guid(TigGuid* guid_ptr);
void gamelib_mod_unload();
bool gamelib_in_reset();
int gamelib_cheat_level_get();
void gamelib_cheat_level_set(int level);
void gamelib_invalidate_rect(TigRect* rect);
bool gamelib_redraw();
void gamelib_renderlock_acquire();
void gamelib_renderlock_release();
void sub_402FC0();
const char* gamelib_current_module_name_get();
void gamelib_current_mode_name_set(const char* name);
bool gamelib_save(const char* name, const char* description);
bool gamelib_load(const char* name);
bool gamelib_delete(const char* name);
const char* gamelib_last_save_name();
bool gamelib_in_save();
bool gamelib_in_load();
void gamelib_set_extra_save_func(GameExtraSaveFunc* func);
void gamelib_set_extra_load_func(GameExtraLoadFunc* func);
void gamelib_savlist_create(GameSaveList* save_list);
void gamelib_modsavlist_create(const char* module, GameSaveList* save_list);
void gamelib_savlist_destroy(GameSaveList* save_list);
void gamelib_savelist_sort(GameSaveList* save_list, GameSaveListOrder order, bool a3);
bool gamelib_saveinfo_init(const char* name, const char* description, GameSaveInfo* save_info);
void gamelib_saveinfo_exit(GameSaveInfo* save_info);
bool gamelib_saveinfo_save(GameSaveInfo* save_info);
bool gamelib_saveinfo_load(const char* name, GameSaveInfo* save_info);
void gamelib_thumbnail_size_set(int width, int height);
int gamelib_game_difficulty_get();
void sub_4045A0();
bool gamelib_copy_version(char* long_version, char* short_version, char* locale);
void gamelib_patch_lvl_set(const char* patch_lvl);
const char* gamelib_get_locale();

#endif /* ARCANUM_GAME_GAMELIB_H_ */
