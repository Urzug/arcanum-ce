#ifndef ARCANUM_GAME_MAP_H_
#define ARCANUM_GAME_MAP_H_

#include "game/context.h"

typedef enum MapType {
    MAP_TYPE_NONE,
    MAP_TYPE_START_MAP,
    MAP_TYPE_SHOPPING_MAP,
    MAP_TYPE_COUNT,
} MapType;

bool map_init(GameInitInfo* init_info);
void map_reset();
void map_resize(GameResizeInfo* resize_info);
bool map_mod_load();
void map_mod_unload();
void map_exit();
void map_ping(unsigned int time);
bool map_save(TigFile* stream);
bool map_load(GameLoadInfo* load_info);
void map_update_view(ViewOptions* view_options);
bool map_new(MapNewInfo* new_map_info);
bool map_open(const char* base_path, const char* save_path, bool a3);
bool map_open_in_game(int map, bool a2, bool a3);
void map_touch(int64_t loc);
void sub_40FED0();
bool map_get_name(int map, char** name);
int map_current_map();
int map_by_type(int map_type);
bool map_get_starting_location(int map, int64_t* x, int64_t* y);
bool map_get_area(int map, int* area);
bool map_get_worldmap(int map, int* worldmap);
bool map_is_clearing_objects();
void map_flush(unsigned int flags);
void sub_4101D0(int64_t location, int64_t a2);
bool map_is_valid();
void map_starting_loc_get(int64_t* loc_ptr);
void map_starting_loc_set(int64_t loc);
void map_paths(char** base_path_ptr, char** save_path_ptr);
bool sub_4102F0(const char* name);
void map_clear_objects();
void map_enable_gender_check();
void map_gender_check();
bool map_list_info_load();
int map_list_info_find(const char* name);
bool map_list_info_set(int index, const char* name, int64_t x, int64_t y, bool is_start_map, int worldmap, int area);
bool map_list_info_add(const char* name, int64_t x, int64_t y, bool is_start_map);
bool map_list_info_add_internal(const char* name, int64_t x, int64_t y, bool is_start_map);

#endif /* ARCANUM_GAME_MAP_H_ */
