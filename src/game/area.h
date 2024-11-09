#ifndef ARCANUM_GAME_AREA_H_
#define ARCANUM_GAME_AREA_H_

#include "game/context.h"

#define AREA_UNKNOWN 0

bool area_init(GameInitInfo* init_info);
void area_reset();
void area_exit();
bool area_mod_load();
void area_mod_unload();
bool area_load(GameLoadInfo* load_info);
bool area_save(TigFile* stream);
int area_get_count();
char* area_get_name(int area);
char* area_get_description(int area);
int64_t area_get_location(int area);
void area_get_wm_offset(int area, int* x, int* y);
bool area_is_known(int64_t pc_obj, int area);
bool area_set_known(int64_t pc_obj, int area);
int area_get_last_known_area(int64_t pc_obj);
void area_reset_last_known_area(int64_t pc_obj);
int area_get_nearest_known_area(int64_t loc, int64_t pc_obj, int64_t range);
int sub_4CB4D0(int64_t loc, bool a2);
int sub_4CB6A0(int64_t obj);

#endif /* ARCANUM_GAME_AREA_H_ */
