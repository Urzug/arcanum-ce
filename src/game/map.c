#include "game/map.h"

#include <stdio.h>

#include "game/anim.h"
#include "game/critter.h"
#include "game/description.h"
#include "game/effect.h"
#include "game/gamelib.h"
#include "game/gsound.h"
#include "game/jumppoint.h"
#include "game/light_scheme.h"
#include "game/light.h"
#include "game/location.h"
#include "game/magictech.h"
#include "game/mes.h"
#include "game/obj_private.h"
#include "game/object.h"
#include "game/oname.h"
#include "game/player.h"
#include "game/proto.h"
#include "game/roof.h"
#include "game/scroll.h"
#include "game/sector.h"
#include "game/skill.h"
#include "game/stat.h"
#include "game/tb.h"
#include "game/teleport.h"
#include "game/terrain.h"
#include "game/tf.h"
#include "game/tile.h"
#include "game/timeevent.h"
#include "game/townmap.h"
#include "game/ui.h"
#include "game/wall.h"
#include "game/wallcheck.h"

#define MAP_LIST_CAPACITY 200
#define MAP_NAME_LENGTH 256
#define MAP_MODULE_COUNT 17
#define SENTINEL 0xBADDBEEF

typedef bool(MapInitFunc)(GameInitInfo* init_info);
typedef void(MapResetFunc)();
typedef bool(MapModuleLoadFunc)();
typedef void(MapModuleUnloadFunc)();
typedef void(MapExitFunc)();
typedef void(MapPingFunc)(unsigned int time);
typedef void(MapUpdateViewFunc)(ViewOptions* view_options);
typedef bool(MapSaveFunc)(TigFile* stream);
typedef bool(MapLoadFunc)(GameLoadInfo* load_info);
typedef void(MapCloseFunc)();
typedef bool(MapNewFunc)(MapNewInfo* new_map_info);
typedef void(MapResizeFunc)(GameResizeInfo* resize_info);

typedef struct MapModule {
    /* 0000 */ const char* name;
    /* 0004 */ MapInitFunc* init_func;
    /* 0008 */ MapResetFunc* reset_func;
    /* 000C */ MapModuleLoadFunc* mod_load_func;
    /* 0010 */ MapModuleUnloadFunc* mod_unload_func;
    /* 0014 */ MapExitFunc* exit_func;
    /* 0018 */ MapPingFunc* ping_func;
    /* 001C */ MapUpdateViewFunc* update_view_func;
    /* 0020 */ MapSaveFunc* save_func;
    /* 0024 */ MapLoadFunc* load_func;
    /* 0028 */ MapNewFunc* new_func;
    /* 002C */ MapCloseFunc* close_func;
    /* 0030 */ MapResizeFunc* resize_func;
} MapModule;

typedef struct MapListInfo {
    char name[MAP_NAME_LENGTH];
    int64_t x;
    int64_t y;
    int worldmap;
    int area;
} MapListInfo;

// See 0x40EA90.
static_assert(sizeof(MapListInfo) == 0x118, "wrong size");

typedef struct MapProperties {
    /* 0000 */ int base_terrain_type;
    /* 0008 */ int64_t width;
    /* 0010 */ int64_t height;
} MapProperties;

static_assert(sizeof(MapProperties) == 0x18, "wrong size");

static void map_close();
static bool map_save_preprocess();
static bool map_save_objects();
static bool map_save_difs();
static bool map_save_dynamic();
static void map_load_postprocess();
static bool map_load_mobile(const char* a1, const char* a2);
static bool map_load_dynamic(const char* name);
static void map_disable_objects();
static void map_obfuscate_name(char* str);
static bool sub_411880(char** str, char* token);
static void map_apply_obj_patch(int64_t obj, char* str);

// 0x59F058
static const char* off_59F058[MAP_TYPE_COUNT] = {
    "NONE",
    "START_MAP",
    "SHOPPING_MAP",
};

// 0x59F068
static MapModule map_modules[] = {
    { "Scroll", scroll_init, scroll_reset, NULL, NULL, scroll_exit, NULL, scroll_update_view, NULL, NULL, NULL, NULL, scroll_resize },
    { "Location", location_init, NULL, NULL, NULL, location_exit, NULL, location_update_view, NULL, NULL, NULL, NULL, location_resize },
    { "Light", light_init, NULL, NULL, NULL, light_exit, NULL, light_update_view, NULL, NULL, NULL, NULL, light_resize },
    { "Tile", tile_init, NULL, NULL, NULL, tile_exit, NULL, tile_update_view, NULL, NULL, NULL, NULL, tile_resize },
    { "Roof", roof_init, NULL, NULL, NULL, roof_exit, NULL, roof_update_view, NULL, NULL, NULL, NULL, roof_resize },
    { "Effect", effect_init, NULL, effect_mod_load, effect_mod_unload, effect_exit, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { "O_Name", o_name_init, NULL, o_name_mod_load, o_name_mod_unload, o_name_exit, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { "Object_Node", object_node_init, NULL, NULL, NULL, object_node_exit, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { "Obj", obj_init, NULL, NULL, NULL, obj_exit, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { "Proto", proto_init, NULL, NULL, NULL, proto_exit, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { "Object", object_init, object_reset, NULL, NULL, object_exit, object_ping, sub_43AA40, NULL, NULL, NULL, object_close, object_resize },
    { "Terrain", terrain_init, terrain_reset, NULL, NULL, terrain_exit, NULL, NULL, NULL, NULL, terrain_new, terrain_close, NULL },
    { "Sector", sector_init, sector_reset, NULL, NULL, sector_exit, NULL, sector_update_view, NULL, NULL, NULL, sub_4CF320, sector_resize },
    { "TB", tb_init, tb_reset, NULL, NULL, tb_exit, tb_ping, tb_update_view, NULL, NULL, NULL, tb_close, tb_resize },
    { "TF", tf_init, tf_reset, NULL, NULL, tf_exit, tf_ping, tf_update_view, NULL, NULL, NULL, tf_map_close, tf_resize },
    { "Wall", wall_init, NULL, NULL, NULL, wall_exit, NULL, wall_update_view, NULL, NULL, NULL, NULL, wall_resize },
    { "JumpPoint", jumppoint_init, jumppoint_reset, NULL, NULL, jumppoint_exit, NULL, jumppoint_update_view, NULL, NULL, jumppoint_new, jumppoint_close, jumppoint_resize },
};

static_assert(sizeof(map_modules) / sizeof(map_modules[0]) == MAP_MODULE_COUNT, "wrong size");

// 0x59F3DC
static char off_59F3DC[] = "*** Deleted Map ***";

// 0x5D11E0
static int64_t qword_5D11E0;

// 0x5D11E8
static bool dword_5D11E8;

// 0x5D11EC
static int dword_5D11EC;

// 0x5D11D0
static bool map_gender_check_enabled;

// 0x5D11D8
static int dword_5D11D8;

// 0x5D11F0
static bool dword_5D11F0;

// 0x5D11F4
static char* map_folder;

// 0x5D11F8
static char* map_name;

// 0x5D11FC
static int dword_5D11FC;

// 0x5D1200
static bool map_editor;

// 0x5D1204
static MapListInfo* map_list_info;

// 0x5D1208
static bool map_in_map_clear_objects;

// 0x5D120C
static int map_list_info_count;

// 0x5D1210
static int dword_5D1210;

// 0x5D1214
static int dword_5D1214[MAP_TYPE_COUNT];

// 0x5D1220
static int dword_5D1220;

// 0x40EA90
bool map_init(GameInitInfo* init_info)
{
    if (dword_5D11F0) {
        return true;
    }

    map_name = (char*)calloc(TIG_MAX_PATH, sizeof(*map_name));
    map_folder = (char*)calloc(TIG_MAX_PATH, sizeof(*map_folder));
    map_list_info = (MapListInfo*)calloc(MAP_LIST_CAPACITY, sizeof(*map_list_info));

    for (int index = 0; index < MAP_MODULE_COUNT; index++) {
        if (map_modules[index].init_func != NULL) {
            if (!map_modules[index].init_func(init_info)) {
                tig_debug_printf("map_init(): init function %d (%s) failed\n", map_modules[index].name);

                while (--index >= 0) {
                    if (map_modules[index].exit_func != NULL) {
                        map_modules[index].exit_func();
                    }
                }

                free(map_name);
                free(map_folder);
                free(map_list_info);

                return false;
            }
        }
    }

    map_gender_check_enabled = 0;
    dword_5D11F0 = true;
    map_editor = init_info->editor;
    dword_5D11EC = 0;

    return true;
}

// 0x40EBA0
void map_reset()
{
    tig_timestamp_t reset_start;
    tig_timestamp_t start;

    if (dword_5D11F0) {
        tig_debug_printf("\nmap_reset: Resetting...\n");
        tig_timer_now(&reset_start);

        tig_debug_printf("map_reset: clearing magictech...");
        tig_timer_now(&start);
        sub_457100();
        tig_debug_printf("done. Time (ms): %d\n", tig_timer_elapsed(start));

        tig_debug_printf("map_reset: interrupting anims...");
        tig_timer_now(&start);
        anim_goal_interrupt_all_goals();
        tig_debug_printf("done. Time (ms): %d\n", tig_timer_elapsed(start));

        tig_debug_printf("map_reset: closing map...");
        tig_timer_now(&start);
        map_close();
        dword_5D1210 = 0;
        tig_debug_printf("done. Time (ms): %d\n", tig_timer_elapsed(start));

        for (int index = 0; index < MAP_MODULE_COUNT; index++) {
            if (map_modules[index].reset_func != NULL) {
                tig_debug_printf("map_reset: Processing Reset Function: %d", index);
                tig_timer_now(&start);
                map_modules[index].reset_func();
                tig_debug_printf(" done. Time (ms): %d.\n", tig_timer_elapsed(start));
            }
        }

        dword_5D11EC = 0;

        tig_debug_printf("map_reset: Done.  Total time: %d ms.\n", tig_timer_elapsed(reset_start));
    }
}

// 0x40ECE0
void map_resize(GameResizeInfo* resize_info)
{
    for (int index = 0; index < MAP_MODULE_COUNT; index++) {
        if (map_modules[index].resize_func != NULL) {
            map_modules[index].resize_func(resize_info);
        }
    }
}

// 0x40ED10
bool map_mod_load()
{
    if (!map_list_info_load()) {
        return false;
    }

    for (int index = 0; index < MAP_MODULE_COUNT; index++) {
        if (map_modules[index].mod_load_func != NULL) {
            if (!map_modules[index].mod_load_func()) {
                tig_debug_printf("map_mod_load(): mod load function %d failed\n", index);

                while (--index >= 0) {
                    if (map_modules[index].mod_unload_func != NULL) {
                        map_modules[index].mod_unload_func();
                    }
                }

                return false;
            }
        }
    }

    return true;
}

// 0x40ED80
void map_mod_unload()
{
    for (int index = MAP_MODULE_COUNT - 1; index >= 0; index--) {
        if (map_modules[index].mod_unload_func != NULL) {
            map_modules[index].mod_unload_func();
        }
    }

    memset(map_list_info, 0, 280);
    map_list_info_count = 0;
}

// 0x40EDC0
void map_exit()
{
    if (dword_5D11F0) {
        map_close();
    }

    for (int index = MAP_MODULE_COUNT - 1; index >= 0; index--) {
        if (map_modules[index].exit_func != NULL) {
            map_modules[index].exit_func();
        }
    }

    free(map_name);
    free(map_folder);
    free(map_list_info);

    dword_5D11F0 = false;
}

// 0x40EE20
void map_ping(unsigned int time)
{
    for (int index = 0; index < MAP_MODULE_COUNT; index++) {
        if (map_modules[index].ping_func != NULL) {
            map_modules[index].ping_func(time);
        }
    }
}

// 0x40EE50
bool map_save(TigFile* stream)
{
    int sentinel = SENTINEL;
    tig_timestamp_t save_start;
    tig_timestamp_t start;
    long start_pos = 0;

    tig_debug_printf("\nmap_save()\n");
    tig_timer_now(&save_start);

    if (!dword_5D11F0) {
        return false;
    }

    tig_debug_printf("map_save: flushing map...");
    tig_timer_now(&start);
    map_flush(1);
    tig_debug_printf("done.  Time (ms): %d\n", tig_timer_elapsed(start));

    tig_debug_printf("map_save: saving map names...");
    tig_timer_now(&start);

    if (tig_file_fputs(map_name, stream) == -1) {
        tig_debug_printf("map_save(): error writing map name.\n");
        return false;
    }

    if (tig_file_fputc('\n', stream) == -1) {
        tig_debug_printf("map_save(): error writing map name delimiter.\n");
        return false;
    }

    if (tig_file_fputs(map_folder, stream) == -1) {
        tig_debug_printf("map_save(): error writing map save folder.\n");
        return false;
    }

    if (tig_file_fputc('\n', stream) == -1) {
        tig_debug_printf("map_save(): error writing map save folder delimiter.\n");
        return false;
    }

    tig_debug_printf("done.  Time (ms): %d\n", tig_timer_elapsed(start));

    // NOTE: This loop is a little bit odd - on any error it stops but does not
    // return false to indicate something bad happened while saving. Could lead
    // to unreadable save games. Check.
    for (int index = 0; index < MAP_MODULE_COUNT; index++) {
        if (map_modules[index].save_func != NULL) {
            tig_debug_printf("map_save: Function %d (%s)", index, map_modules[index].name);
            tig_timer_now(&start);

            if (!map_modules[index].save_func(stream)) {
                tig_debug_printf("map_save: save function %d (%s) failed\n", index, map_modules[index].name);
                break;
            }

            int pos;
            tig_file_fgetpos(stream, &pos);
            tig_debug_printf(" wrote to: %lu, Total: (%lu), Time (ms): %d\n",
                pos,
                pos - start_pos,
                tig_timer_elapsed(start));
            start_pos = pos;

            if (tig_file_fwrite(&sentinel, sizeof(sentinel), 1, stream) != 1) {
                tig_debug_printf("map_save(): ERROR: Sentinel Failed to Save!");
                break;
            }
        }
    }

    tig_debug_printf("map_save: Save Complete.  Total time: %d ms.\n", tig_timer_elapsed(save_start));

    return true;
}

// 0x40F080
bool map_load(GameLoadInfo* load_info)
{
    int sentinel;
    tig_timestamp_t load_start;
    tig_timestamp_t start;
    long start_pos = 0;

    tig_debug_printf("\nmap_load\n");
    tig_timer_now(&load_start);

    if (!dword_5D11F0) {
        return false;
    }

    tig_debug_printf("map_load: retrieving map names...");
    tig_timer_now(&start);

    char name[TIG_MAX_PATH];
    if (tig_file_fgets(name, sizeof(name), load_info->stream) == NULL) {
        map_close();
        return false;
    }

    const size_t name_length = strlen(name);
    if (name[name_length - 1] == '\n') {
        name[name_length - 1] = '\0';
    }

    char folder[TIG_MAX_PATH];
    if (tig_file_fgets(folder, sizeof(folder), load_info->stream) == NULL) {
        map_close();
        return false;
    }

    const size_t folder_length = strlen(folder);
    if (folder[folder_length - 1] == '\n') {
        folder[folder_length - 1] = '\0';
    }

    tig_debug_printf("done.  Time (ms): %d\n", tig_timer_elapsed(start));

    for (int index = 0; index < MAP_MODULE_COUNT; index++) {
        if (map_modules[index].load_func != NULL) {
            tig_debug_printf("map_load: Function %d (%s)", index, map_modules[index].name);
            tig_timer_now(&start);

            if (!map_modules[index].load_func(load_info)) {
                tig_debug_printf("map_load(): error calling map load func %d (%s)\n", index, map_modules[index].name);
                map_close();
                return false;
            }

            if (tig_file_fread(&sentinel, sizeof(sentinel), 1, load_info->stream) != 1) {
                tig_debug_printf("map_load: ERROR: Load Sentinel Failed to Load!\n");
                // FIXME: Missing map_close?
                return false;
            }

            if (sentinel != SENTINEL) {
                tig_debug_printf("map_load: ERROR: Load Sentinel Failed to Match!\n");
                // FIXME: Missing map_close?
                return false;
            }

            int pos;
            tig_file_fgetpos(load_info->stream, &pos);
            tig_debug_printf(" read to: %lu, Total: (%lu), Time (ms): %d\n",
                pos,
                start_pos - pos,
                tig_timer_elapsed(start));
            start_pos = pos;
        }
    }

    tig_debug_printf("map_load: opening map...");
    tig_timer_now(&start);
    bool success = map_open(name, folder, 1);
    tig_debug_printf("done.  Time (ms): %d\n", tig_timer_elapsed(start));

    tig_debug_printf("map_load: Load Complete.  Total time: %d ms.\n", tig_timer_elapsed(load_start));

    return success;
}

// 0x40F320
void map_update_view(ViewOptions* view_options)
{
    if (dword_5D11F0) {
        for (int index = MAP_MODULE_COUNT - 1; index >= 0; index--) {
            if (map_modules[index].update_view_func != NULL) {
                map_modules[index].update_view_func(view_options);
            }
        }
    }
}

// 0x40F350
bool map_new(MapNewInfo* new_map_info)
{
    int index;
    char path[TIG_MAX_PATH];
    TigFile* stream;
    MapProperties map_properties;

    if (!dword_5D11F0) {
        return false;
    }

    map_close();

    if (!tig_file_is_directory(new_map_info->name)) {
        if (!tig_file_mkdir(new_map_info->name)) {
            return false;
        }
    }

    if (!sub_52E040(new_map_info->name)) {
        return false;
    }

    for (index = 0; index < MAP_MODULE_COUNT; index++) {
        if (map_modules[index].new_func != NULL) {
            if (!map_modules[index].new_func(new_map_info)) {
                tig_debug_printf("map_new(): Error calling new func %d\n", index);
                map_close();
                return false;
            }
        }
    }

    sprintf(path, "%s\\map.prp", new_map_info->name);

    stream = tig_file_fopen(path, "wb");
    if (stream == NULL) {
        tig_debug_printf("Error creating map properties file %s\n", path);
        return false;
    }

    map_properties.base_terrain_type = new_map_info->base_terrain_type;
    map_properties.width = new_map_info->width << 6;
    map_properties.height = new_map_info->height << 6;
    if (!tig_file_fwrite(&map_properties, sizeof(map_properties), 1, stream) != 1) {
        tig_debug_printf("Error writing map properties file %s\n", path);
        tig_file_fclose(stream);
        return false;
    }

    tig_file_fclose(stream);

    return map_open(new_map_info->name, new_map_info->folder, false);
}

// 0x40F4E0
bool map_open(const char* a1, const char* a2, bool a3)
{
    tig_timestamp_t start_timestamp;
    tig_timestamp_t timestamp;
    tig_duration_t duration;
    char path[TIG_MAX_PATH];
    TigFile* stream;
    MapProperties map_properties;
    char tmp[80];
    char* pch;
    int map;
    MapNewInfo new_map_info;

    if (!dword_5D11F0) {
        return false;
    }

    tig_debug_printf("map_open: map_close()...");
    tig_timer_now(&timestamp);
    start_timestamp = timestamp;
    map_close();
    duration = tig_timer_elapsed(timestamp);
    tig_debug_printf("done.  Time (ms): %d\n", duration);

    if (!tig_file_is_directory(a1)) {
        tig_debug_printf("Error opening map %s: folder does not exist\n", a1);
        return false;
    }

    tig_file_mkdir(a2);
    if (!a3) {
        sub_52E040(a2);
    }

    tig_debug_printf("map_open: reading properties file...");
    tig_timer_now(&timestamp);
    sprintf(path, "%s\\map.prp", a1);
    stream = tig_file_fopen(path, "rb");
    if (stream == NULL) {
        tig_debug_printf("Error opening map properties file %s\n", path);
        return false;
    }
    if (tig_file_fread(&map_properties, sizeof(map_properties), 1, stream) != 1) {
        tig_debug_printf("Error reading map properties file %s\n", path);
        tig_file_fclose(stream);
        return false;
    }
    tig_file_fclose(stream);
    duration = tig_timer_elapsed(timestamp);
    tig_debug_printf("done.  Time (ms): %d\n", duration);

    tig_debug_printf("map_open: reading start location...");
    sprintf(path, "%s\\startloc.txt", a1);
    if (tig_file_exists(path, NULL)) {
        int64_t x;
        int64_t y;

        stream = tig_file_fopen(path, "rt");
        if (stream == NULL) {
            tig_debug_printf("Error opening map start location (file exists, can't open): %s\n", path);
            return false;
        }

        if (tig_file_fgets(tmp, 80, stream) == NULL) {
            // FIXME: Leaking stream.
            tig_debug_printf("Error reading map start location (x) file %s\n", path);
            return false;
        }
        x = _atoi64(tmp);

        if (tig_file_fgets(tmp, 80, stream) == NULL) {
            // FIXME: Leaking stream.
            tig_debug_printf("Error reading map start location (y) file %s\n", path);
            return false;
        }
        y = _atoi64(tmp);

        qword_5D11E0 = x | y;

        tig_file_fclose(stream);
    }
    duration = tig_timer_elapsed(timestamp);
    tig_debug_printf("done.  Time (ms): %d\n", duration);

    tig_debug_printf("map_open: loading mobile objects...");
    tig_timer_now(&timestamp);
    if (!map_load_mobile(a1, a2)) {
        tig_debug_println("Error reading mobile objects");
        map_close();
        return false;
    }
    duration = tig_timer_elapsed(timestamp);
    tig_debug_printf("done.  Time (ms): %d\n", duration);

    tig_debug_printf("map_open: loading mobile dynamic objects...");
    tig_timer_now(&timestamp);
    if (!map_editor) {
        if (!map_load_dynamic(a2)) {
            tig_debug_println("Error reading dynamic mobile objects\n");
                map_close();
                return false;
        }
    }
    duration = tig_timer_elapsed(timestamp);
    tig_debug_printf("done.  Time (ms): %d\n", duration);

    tig_debug_printf("map_open: map_load_postprocess()...");
    tig_timer_now(&timestamp);
    map_load_postprocess();
    duration = tig_timer_elapsed(timestamp);
    tig_debug_printf("done.  Time (ms): %d\n", duration);

    strcpy(map_name, a1);
    strcpy(map_folder, a2);
    tig_file_mkdir(map_folder);
    if (!a3) {
        sub_52E040(map_folder);
    }

    tig_debug_printf("map_open: loading terrain...");
    tig_timer_now(&timestamp);
    if (!terrain_open(map_name, map_folder)) {
        new_map_info.name = map_name;
        new_map_info.folder = map_name; // FIXME: Using name as folder.
        new_map_info.base_terrain_type = map_properties.base_terrain_type;
        new_map_info.width = map_properties.width >> 6;
        new_map_info.height = map_properties.height >> 6;
        terrain_new(&new_map_info);

        if (!terrain_open(map_name, map_folder)) {
            tig_debug_println("Error: terrain_open failed even after creating a new terrain");
            map_close();
            return false;
        }
    }
    duration = tig_timer_elapsed(timestamp);
    tig_debug_printf("done.  Time (ms): %d\n", duration);

    tig_debug_printf("map_open: loading jump points...");
    tig_timer_now(&timestamp);
    if (!jumppoint_open(map_name, map_folder)) {
        new_map_info.name = map_name;
        new_map_info.folder = map_folder;
        new_map_info.base_terrain_type = map_properties.base_terrain_type;
        new_map_info.width = map_properties.width >> 6;
        new_map_info.height = map_properties.height >> 6;
        jumppoint_new(&new_map_info);

        if (!jumppoint_open(map_name, map_folder)) {
            tig_debug_println("Error: jumppoint_open failed even after creating a new file");
            map_close();
            return false;
        }
    }
    duration = tig_timer_elapsed(timestamp);
    tig_debug_printf("done.  Time (ms): %d\n", duration);

    tig_debug_printf("map_open: sector blocking...");
    tig_timer_now(&timestamp);
    if (!sector_block_load(map_name, map_folder)) {
        sector_block_init();
    }
    duration = tig_timer_elapsed(timestamp);
    tig_debug_printf("done.  Time (ms): %d\n", duration);

    tig_debug_printf("map_open: setting initial map location and state...");
    tig_timer_now(&timestamp);
    location_limits_set(map_properties.width, map_properties.height);
    sector_limits_set(map_properties.width >> 6, map_properties.height >> 6);
    sector_map_name_set(a1, a2);
    location_origin_set(sub_4B9810());
    dword_5D1210 = 0;
    pch = strrchr(a1, '\\');
    map = map_list_info_find(pch != NULL ? pch + 1 : a1);
    if (map != -1) {
        dword_5D1210 = map + 1;
    }
    duration = tig_timer_elapsed(timestamp);
    tig_debug_printf("done.  Time (ms): %d\n", duration);

    tig_debug_printf("map_open: parsing extension file...");
    tig_timer_now(&timestamp);
    sub_4115D0(a1);
    duration = tig_timer_elapsed(timestamp);
    tig_debug_printf("done.  Time (ms): %d\n", duration);

    tig_debug_printf("map_open: map_disable_objects()...");
    tig_timer_now(&timestamp);
    map_disable_objects();
    duration = tig_timer_elapsed(timestamp);
    tig_debug_printf("done.  Time (ms): %d\n", duration);

    tig_debug_printf("map_open: map_gender_check()...");
    tig_timer_now(&timestamp);
    map_gender_check();
    dword_5D11EC = 0;
    if (dword_5D1210 != 0 && dword_5D1214[MAP_TYPE_START_MAP] == dword_5D1210) {
        dword_5D11EC = 1;
    }
    duration = tig_timer_elapsed(timestamp);
    tig_debug_printf("done.  Time (ms): %d\n", duration);

    duration = tig_timer_elapsed(start_timestamp);
    tig_debug_printf("map_open(): Done.  Total time: %d ms.\n", duration);

    dword_5D11E8 = 0;
    dword_5D11FC = 1;

    return true;
}

// 0x40FC70
bool map_open_in_game(int map, bool a2, bool a3)
{
    MapListInfo* info;
    char path1[TIG_MAX_PATH];
    char path2[TIG_MAX_PATH];

    if (!dword_5D11F0) {
        return false;
    }

    if (map > map_list_info_count || map == 0) {
        return false;
    }

    info = &(map_list_info[map - 1]);
    if (dword_5D11FC && !a3) {
        map_flush(0);
    }

    if (!obj_validate_system(1)) {
        tig_debug_println("Object system validate failed pre-load in map_open_in_game.");
        tig_message_post_quit(0);
        // FIXME: Execution continues after quit.
    }

    sprintf(path1, "maps\\%s", info->name);
    sprintf(path2, "%s\\maps\\%s", "Save\\Current", info->name);

    tig_debug_printf("Loading Map: %s\n", path1);
    gsound_stop_all(0);

    if (!map_open(path1, path2, 1)) {
        return false;
    }

    qword_5D11E0 = info->x | (info->y << 32);
    location_origin_set(qword_5D11E0);

    if (a2) {
        sub_40FE00(qword_5D11E0);
    }

    sub_45C580();
    sub_4605C0();

    if (!obj_validate_system(1)) {
        tig_debug_println("Object system validate failed post-load in map_open_in_game.");
        tig_message_post_quit(0);
    }

    return true;
}

// 0x40FE00
void sub_40FE00(int64_t loc)
{
    int64_t sector_id;
    int64_t x;
    int64_t y;
    int dx;
    int dy;
    Sector* sector;

    sector_id = sector_id_from_loc(loc);
    x = SECTOR_X(sector_id) - 1;
    y = SECTOR_Y(sector_id) - 1;

    for (dy = 0; dy < 3; dy++) {
        for (dx = 0; dx < 3; dx++) {
            sector_id = SECTOR_MAKE(x + dx, y + dy);
            if (sector_lock(sector_id, &sector)) {
                sector_unlock(sector_id);
            }
        }
    }
}

// 0x40FED0
void sub_40FED0()
{
}

// 0x40FEE0
bool map_get_name(int map, char** name)
{
    if (map > 0 && map <= map_list_info_count) {
        *name = map_list_info[map - 1].name;
        return true;
    } else {
        *name = NULL;
        return false;
    }
}

// 0x40FF40
int sub_40FF40()
{
    return dword_5D1210;
}

// 0x40FF50
int sub_40FF50(int map_type)
{
    if (map_type >= 0 && map_type < MAP_TYPE_COUNT) {
        return dword_5D1214[map_type];
    } else {
        return 0;
    }
}

// 0x40FF70
bool map_get_starting_location(int map, int64_t* x, int64_t* y)
{
    if (map > 0 && map <= map_list_info_count) {
        *x = map_list_info[map - 1].x;
        *y = map_list_info[map - 1].y;
        return true;
    } else {
        *x = 0;
        *y = 0;
        return false;
    }
}

// 0x40FFE0
bool map_get_area(int map, int* area)
{
    if (map > 0 && map <= map_list_info_count) {
        *area = map_list_info[map - 1].area;
        return true;
    } else {
        *area = 0;
        return false;
    }
}

// 0x410020
bool map_get_worldmap(int map, int* worldmap)
{
    if (map > 0 && map <= map_list_info_count) {
        *worldmap = map_list_info[map - 1].worldmap;
        return true;
    } else {
        *worldmap = -1;
        return false;
    }
}

// 0x410060
bool map_is_clearing_objects()
{
    return map_in_map_clear_objects;
}

// 0x410070
void map_flush(unsigned int flags)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;

    wallcheck_flush();

    if (!map_save_preprocess()) {
        tig_debug_println("Error: map_save_preprocess failed in map_flush.");
    }

    if (map_editor) {
        if (!map_save_objects()) {
            tig_debug_println("Error: map_save_objects failed in map_flush.");
        }
    } else {
        if (!map_save_difs()) {
            tig_debug_println("Error: map_save_difs failed in map_flush.");
        }

        if (!map_save_dynamic()) {
            tig_debug_println("Error: map_save_dynamic failed in map_flush.");
        }
    }

    map_load_postprocess();
    sector_flush(flags);
    jumppoint_flush();
    terrain_flush();
    townmap_flush();

    if (map_editor) {
        sprintf(path, "%s\\startloc.txt", map_folder);
        if (qword_5D11E0 != 0) {
            stream = tig_file_fopen(path, "wt");
            if (stream != NULL) {
                tig_file_fprintf(stream,
                    "%I64d\n%I64d\n",
                    LOCATION_GET_X(qword_5D11E0),
                    LOCATION_GET_Y(qword_5D11E0));
            } else {
                tig_debug_printf("Error could not open map start location file for writing %s\n", path);
            }
        } else {
            tig_file_remove(path);
        }
    }
}

// 0x4101D0
void sub_4101D0(int64_t location, int64_t a2)
{
    JumpPoint jumppoint;
    TeleportData teleport_data;

    if (dword_5D1220 < 9) {
        if (jumppoint_find_by_location(location, &jumppoint)) {
            dword_5D1220++;

            teleport_data.flags = 0;
            teleport_data.obj = a2;
            teleport_data.loc = jumppoint.field_18;
            teleport_data.map = jumppoint.field_10;
            teleport_do(&teleport_data);

            dword_5D1220--;
        }
    }
}

// 0x410270
int sub_410270()
{
    return dword_5D11FC;
}

// 0x410280
void sub_410280(int64_t* location)
{
    if (location != NULL) {
        *location = qword_5D11E0;
    }
}

// 0x4102C0
void sub_4102C0(char** name, char** folder)
{
    if (name != NULL) {
        *name = map_name;
    }

    if (folder != NULL) {
        *folder = map_folder;
    }
}

// 0x4102F0
bool sub_4102F0(const char* name)
{
    GUID guid;
    GUID file_guid;
    TigFile* stream;
    char path[TIG_MAX_PATH];
    char* pch;
    TigFileList file_list;
    unsigned int index;
    int64_t obj;

    if (!gamelib_mod_guid(&guid)) {
        tig_debug_printf("Attempt to retrieve mod GUID when no mod is loaded\n");
        return true;
    }

    strcpy(path, "maps\\");
    strcat(path, name);
    map_obfuscate_name(&(path[5]));

    if (tig_file_exists(path, NULL)) {
        stream = tig_file_fopen(path, "rb");
        if (stream == NULL) {
            tig_debug_printf("Error opening file %s\n", path);
            return false;
        }

        if (tig_file_fread(&file_guid, sizeof(file_guid), 1, stream) != 1) {
            // FIXME: Leaking `stream`.
            tig_debug_printf("Error reading GUID from file %s\n", path);
            return false;
        }

        tig_file_fclose(stream);

        if (IsEqualGUID(&file_guid, &guid)) {
            return true;
        }
    }

    tig_file_mkdir("maps");

    stream = tig_file_fopen(path, "wb");
    if (stream == NULL) {
        tig_debug_printf("Error creating file %s\n", path);
        return false;
    }

    if (tig_file_fwrite(&guid, sizeof(guid), 1, stream) != 1) {
        // FIXME: Leaking `stream`.
        tig_debug_printf("Error writing mod GUID to file %s\n", path);
        return false;
    }

    strcpy(path, "maps\\");
    strcat(path, name);
    strcat(path, "\\");

    pch = &(path[strlen(path)]);
    strcpy(pch, "*.mob");

    tig_file_list_create(&file_list, path);

    for (index = 0; index < file_list.count; index++) {
        strcpy(pch, file_list.entries[index].path);
        if (!objf_solitary_read(&obj, path)) {
            // FIXME: Leaking `stream`.
            tig_file_list_destroy(&file_list);
            tig_debug_printf("Error reading object %s\n", path);
            return false;
        }

        if (!obj_write(stream, obj)) {
            // FIXME: Leaking `stream`.
            tig_file_list_destroy(&file_list);
            tig_debug_printf("Error writing object to mobile objects file.\n", path);
            return false;
        }

        sub_405BF0(obj);
    }

    tig_file_list_destroy(&file_list);
    tig_file_fclose(stream);
    return true;
}

// 0x4106D0
void map_close()
{
    int index;

    if (!dword_5D11E8) {
        dword_5D11E8 = true;

        wallcheck_flush();
        gsound_flush();

        dword_5D11FC = false;

        for (index = 0; index < MAP_MODULE_COUNT; index++) {
            if (map_modules[index].close_func != NULL) {
                map_modules[index].close_func();
            }
        }

        map_clear_objects();
    }
}

// 0x410720
bool map_save_preprocess()
{
    int64_t obj;
    int iter;

    if (obj_inst_first(&obj, &iter)) {
        do {
            if (!sub_43D990(obj)) {
                sub_4064B0(obj);
            }
        } while (obj_inst_next(&obj, &iter));
    }

    return true;
}

// 0x410780
bool map_save_objects()
{
    int64_t obj;
    int iter;

    if (obj_inst_first(&obj, &iter)) {
        do {
            if (!sub_43D990(obj)
                && (obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DYNAMIC)
                && obj_is_modified(obj)) {
                if (!objf_solitary_write(obj, map_folder, ".mob")) {
                    return false;
                }
            }
        } while (obj_inst_next(&obj, &iter));
    }

    return true;
}

// 0x410830
bool map_save_difs()
{
    int cnt = 0;
    char path1[TIG_MAX_PATH];
    char path2[TIG_MAX_PATH];
    TigFile* stream1;
    TigFile* stream2;
    int size;
    int64_t obj;
    int iter;
    ObjectID oid;

    sprintf(path1, "%s\\mobile.md", map_folder);
    stream1 = tig_file_fopen(path1, "wb");
    if (stream1 == NULL) {
        tig_debug_printf("Error opening file %s for writing...\n", path1);
        tig_debug_printf("Cannot save mobile object differences\n");
        return false;
    }

    sprintf(path2, "%s\\mobile.des", map_folder);
    stream2 = tig_file_fopen(path2, "ab");
    if (stream2 == NULL) {
        tig_file_fclose(stream1);
        tig_debug_printf("Error opening file %s for writing...\n", path2);
        tig_debug_printf("Cannot save mobile object differences\n");
        return false;
    }

    size = tig_file_filelength(stream2);

    if (obj_inst_first(&obj, &iter)) {
        do {
            if (!sub_43D990(obj)
                && (obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DYNAMIC) == 0
                && obj_is_modified(obj)) {
                oid = sub_407EF0(obj);
                if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & (OF_EXTINCT | OF_DESTROYED)) != 0) {
                    if (tig_file_fwrite(&oid, sizeof(oid), 1, stream2) != 1) {
                        tig_file_fclose(stream2);
                        tig_file_fclose(stream1);
                        tig_debug_printf("Error writing object id to file %s\n", path2);
                        tig_debug_printf("Cannot save mobile object differences\n");
                        return false;
                    }
                    size += sizeof(oid);
                } else {
                    if (tig_file_fwrite(&oid, sizeof(oid), 1, stream1) != 1) {
                        tig_file_fclose(stream2);
                        tig_file_fclose(stream1);
                        tig_debug_printf("Error writing object id to file %s\n", path1);
                        tig_debug_printf("Cannot save mobile object differences\n");
                        return false;
                    }

                    if (!obj_dif_write(stream1, obj)) {
                        tig_file_fclose(stream2);
                        tig_file_fclose(stream1);
                        tig_debug_printf("Error writing object differences to file %s\n", path1);
                        tig_debug_printf("Cannot save mobile object differences\n");
                        return false;
                    }

                    cnt++;
                }
            }
        } while (obj_inst_next(&obj, &iter));
    }

    tig_file_fclose(stream2);

    if (size < sizeof(oid)) {
        tig_file_remove(path2);
    }

    tig_file_fclose(stream1);

    if (cnt == 0) {
        tig_file_remove(path1);
    }

    return true;
}

// 0x410B20
bool map_save_dynamic()
{
    int cnt = 0;
    char path[TIG_MAX_PATH];
    TigFile* stream;
    int64_t obj;
    int iter;
    unsigned int flags;

    sprintf(path, "%s\\mobile.mdy", map_folder);
    stream = tig_file_fopen(path, "wb");
    if (stream == NULL) {
        tig_debug_printf("Error opening mobile dynamic objects file %s to write.\n", path);
        return false;
    }

    if (obj_inst_first(&obj, &iter)) {
        do {
            if (!sub_43D990(obj)) {
                flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
                if ((flags & OF_DYNAMIC) != 0 && (flags & (OF_EXTINCT | OF_DESTROYED)) == 0) {
                    if (!obj_write(stream, obj)) {
                        tig_debug_printf("Error saving object to mobile dynamic objects file %s.\n", path);
                        tig_file_fclose(stream);
                        tig_file_remove(path);
                        return false;
                    }
                    cnt++;
                }
            }
        } while (obj_inst_next(&obj, &iter));
    }

    tig_file_fclose(stream);

    if (cnt == 0) {
        tig_file_remove(path);
    }

    return true;
}

// 0x410C50
void map_load_postprocess()
{
    int64_t obj;
    int iter;
    unsigned int flags;
    TimeEvent timeevent;

    if (obj_inst_first(&obj, &iter)) {
        do {
            if (!sub_43D990(obj)) {
                sub_406520(obj);
            }

            flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
            if ((flags & OF_TELEPORTED) != 0) {
                timeevent.type = TIMEEVENT_TYPE_TELEPORTED;
                timeevent.params[0].object_value = obj;
                sub_45B820(&timeevent);

                flags &= ~OF_TELEPORTED;
                obj_field_int32_set(obj, OBJ_F_FLAGS, flags);
            }
        } while (obj_inst_next(&obj, &iter));
    }
}

// 0x410D10
bool map_load_mobile(const char* a1, const char* a2)
{
    char path[TIG_MAX_PATH];
    char path2[TIG_MAX_PATH];
    char path3[TIG_MAX_PATH];
    char* fname;
    TigFileList file_list;
    unsigned int idx;
    ObjectID oid;
    int64_t obj;
    TigFile* stream;
    GUID guid;

    if (map_editor) {
        sprintf(path, "%s\\*.mob", a1);
        tig_file_list_create(&file_list, path);
        for (idx = 0; idx < file_list.count; idx++) {
            sprintf(path, "%s\\%s", a2, file_list.entries[idx].path);
            if (!tig_file_exists(path, NULL)) {
                strcpy(&(path[strlen(path) - 4]), ".del");
                if (!tig_file_exists(path, NULL)) {
                    sprintf(path, "%s\\%s", a1, file_list.entries[idx].path);
                    if (!objf_solitary_read(&obj, path)) {
                        tig_file_list_destroy(&file_list);
                        tig_debug_printf("Error reading object %s\n", path);
                        return false;
                    }
                }
            }
        }
        tig_file_list_destroy(&file_list);

        sprintf(path, "%s\\*.mob", a2);
        tig_file_list_create(&file_list, path);
        for (idx = 0; idx < file_list.count; idx++) {
            sprintf(path, "%s\\%s", a2, file_list.entries[idx].path);
            if (!objf_solitary_read(&obj, path)) {
                tig_file_list_destroy(&file_list);
                tig_debug_printf("Error reading object %s\n", path);
                return false;
            }
        }
    }

    strcpy(path2, a1);
    map_obfuscate_name(&(path2[5]));

    strcpy(path3, a2);
    strcat(path3, "\\");
    fname = &(path3[strlen(path3)]);

    stream = tig_file_fopen(path2, "rb");
    if (stream == NULL) {
        tig_debug_printf("Error opening file %s\n", path2);
        return false;
    }

    if (tig_file_fread(&guid, sizeof(guid), 1, stream) != 1) {
        // FIX: Release `stream`.
        tig_file_fclose(stream);

        tig_debug_printf("Error reading GUID from file %s\n", path2);
        return false;
    }

    while (obj_read(stream, &obj)) {
    }

    if (tig_file_feof(stream) == 0) {
        // FIX: Release `stream`.
        tig_file_fclose(stream);

        tig_debug_printf("Error reading object from file %s\n", path2);
        return false;
    }

    // FIX: Release `stream`.
    tig_file_fclose(stream);

    strcpy(fname, "mobile.md");
    if (tig_file_exists(path3, NULL)) {
        stream = tig_file_fopen(path3, "rb");
        if (stream == NULL) {
            tig_debug_printf("Error opening differences file %s for reading\n", path3);
            return false;
        }

        while (tig_file_fread(&oid, sizeof(oid), 1, stream) == 1) {
            obj = objp_perm_lookup(oid);
            if (obj == OBJ_HANDLE_NULL) {
                tig_file_fclose(stream);
                tig_debug_printf("Error retrieving object handle to apply differences\n");
                return false;
            }

            if (!obj_dif_read(stream, obj)) {
                tig_file_fclose(stream);
                tig_debug_printf("Error reading object differences\n");
                return false;
            }

            if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_EXTINCT) != 0) {
                sub_405BF0(obj);
            }
        }

        if (tig_file_feof(stream) == 0) {
            tig_file_fclose(stream);
            tig_debug_printf("Error reading differences from file %s\n", path3);
            return false;
        }

        tig_file_fclose(stream);
    }

    strcpy(fname, "mobile.des");
    if (tig_file_exists(path3, NULL)) {
        stream = tig_file_fopen(path3, "rb");

        if (stream != NULL) {
            while (tig_file_fread(&oid, sizeof(oid), 1, stream) == 1) {
                obj = objp_perm_lookup(oid);
                if (obj != OBJ_HANDLE_NULL) {
                    sub_405BF0(obj);
                }
            }

            tig_file_fclose(stream);
        }
    }

    if (tig_file_exists("Rules\\map.patch", NULL)) {
        stream = tig_file_fopen("Rules\\map.patch", "rt");
        if (stream != NULL) {
            char str[4096];
            int line;
            char* end;

            line = 0;
            while (tig_file_fgets(str, sizeof(str), stream) != NULL) {
                line++;

                if (str[0] != '#' && str[0] != '\n') {
                    end = &(str[strlen(str) - 1]);
                    if (*end != '\n'
                        && !tig_file_feof(stream)) {
                        tig_debug_printf("Error reading map.patch line %d is too long.\n", line);
                        break;
                    }

                    *end = '\0';
                    if (objid_id_from_str(&oid, str)) {
                        obj = objp_perm_lookup(oid);
                        if (obj != OBJ_HANDLE_NULL) {
                            map_apply_obj_patch(obj, strchr(str, ':'));
                        }
                    } else {
                        tig_debug_printf("Malformed line %d: \"%s\"\n", line, str);
                    }
                }
            }

            tig_file_fclose(stream);
        }
    }

    return true;
}

// 0x411450
bool map_load_dynamic(const char* name)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;
    int64_t obj;

    sprintf(path, "%s\\mobile.mdy", name);

    if (tig_file_exists(path, NULL)) {
        stream = tig_file_fopen(path, "rb");
        if (stream == NULL) {
            tig_debug_printf("Error opening mobile dynamic objects file %s for reading.\n", path);
            return false;
        }

        while (obj_read(stream, &obj)) {
            // Noop.
        }

        if (tig_file_feof(stream) == 0) {
            tig_file_fclose(stream);
            tig_debug_printf("Error reading mobile dynamic objects from file %s\n", path);
        }

        tig_file_fclose(stream);
    }

    return true;
}

// 0x411520
void map_clear_objects()
{
    int64_t obj;
    int iter;

    if (map_in_map_clear_objects) {
        tig_debug_println("Waring: map_clear_objects attempting to recurse.");
    } else {
        map_in_map_clear_objects = true;

        timeevent_clear_for_map_close();

        if (obj_inst_first(&obj, &iter)) {
            do {
                if (!sub_43D990(obj)) {
                    sub_43CF70(obj);
                    sub_43CFF0(obj);
                }
            } while (obj_inst_next(&obj, &iter));
        }

        sub_4D0B40();
        sub_4E5300();

        map_in_map_clear_objects = false;
    }
}

// 0x4115D0
void sub_4115D0(const char* name)
{
    char path[TIG_MAX_PATH];
    strcpy(path, name);
    strcat(path, "\\mapinfo.txt");

    TigFile* stream = tig_file_fopen(path, "rt");
    if (stream != NULL) {
        char key[64];
        while (tig_file_fgets(path, sizeof(path), stream) != NULL) {
            char* str = path;

            tig_str_parse_set_separator(':');
            tig_str_parse_str_value(&str, key);

            if (_strcmpi(key, "LightScheme") == 0) {
                int light_scheme;

                tig_str_parse_value(&str, &light_scheme);
                light_scheme_set_map_default(light_scheme);
                light_scheme_set(LIGHT_SCHEME_MAP_DEFAULT, light_scheme_get_hour());
            } else if (_strcmpi(key, "SoundScheme") == 0) {
                int music_scheme_idx;
                int ambient_scheme_idx;

                tig_str_parse_set_separator(',');
                tig_str_parse_value(&str, &music_scheme_idx);
                tig_str_parse_value(&str, &ambient_scheme_idx);

                if (!map_editor) {
                    gsound_play_scheme(music_scheme_idx, ambient_scheme_idx);
                }
            }
        }

        tig_file_fclose(stream);
    }
}

// 0x411750
void map_disable_objects()
{
    int64_t obj;
    int iter;
    int64_t location;
    int64_t sector_id;
    unsigned int flags;

    if (obj_inst_first(&obj, &iter)) {
        do {
            if (!sub_43D990(obj)) {
                location = obj_field_int64_get(obj, OBJ_F_LOCATION);
                sector_id = sector_id_from_loc(location);
                if (!sub_4D0DE0(sector_id) && !player_is_local_pc_obj(obj)) {
                    if (critter_pc_leader_get(obj) != player_get_local_pc_obj()) {
                        flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
                        flags |= OF_OFF;
                        obj_field_int32_set(obj, OBJ_F_FLAGS, flags);
                    }
                }
            }
        } while (obj_inst_next(&obj, &iter));
    }
}

// 0x411830
void map_obfuscate_name(char* str)
{
    unsigned char* pch;

    pch = (unsigned char*)str;
    while (*pch != '\0') {
        if (*pch >= 'A' && *pch <= 'Z') {
            *pch += 13;
            if (*pch > 'Z') {
                *pch += 'A' - 'Z' + 1;
            }
        } else if (*pch >= 'a' && *pch <= 'z') {
            *pch += 13;
            if (*pch > 'z') {
                *pch += 'a' - 'z' + 1;
            }
        }
        pch++;
    }

    strrev(str);
}

// 0x411880
bool sub_411880(char** str, char* token)
{
    if (token == NULL) {
        return false;
    }

    *token = '\0';

    if (str == NULL) {
        return false;
    }

    while (**str != '\0' && isspace(**str)) {
        (*str)++;
    }

    if (**str == '\0') {
        return false;
    }

    while (**str != '\0' && !isspace(**str)) {
        *token++ = **str;
        (*str)++;
    }
    *token = '\0';

    return true;
}

// 0x411940
void map_apply_obj_patch(int64_t obj, char* str)
{
    char key[4096];
    char value[4096];
    unsigned int flags;
    int index;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (*str != ':') {
        return;
    }

    // Consume semicolon.
    str++;

    while (sub_411880(&str, key)) {
        if (strcmpi(key, "internal_name") == 0) {
            sub_411880(&str, value);
            obj_field_int32_set(obj, OBJ_F_NAME, atoi(value));
        } else if (strcmpi(key, "known_name") == 0) {
            sub_411880(&str, value);
            obj_field_int32_set(obj, OBJ_F_DESCRIPTION, atoi(value));
        } else if (strcmpi(key, "alignment") == 0) {
            sub_411880(&str, value);
            stat_base_set(obj, STAT_ALIGNMENT, atoi(value));
        } else if (strcmpi(key, "origin") == 0) {
            sub_411880(&str, value);
            obj_field_int32_set(obj, OBJ_F_NPC_ORIGIN, atoi(value));
        } else if (strcmpi(key, "notify_npc") == 0) {
            sub_411880(&str, value);
            switch (obj_field_int32_get(obj, OBJ_F_TYPE)) {
            case OBJ_TYPE_PORTAL:
                obj_field_int32_set(obj, OBJ_F_PORTAL_NOTIFY_NPC, atoi(value));
                break;
            case OBJ_TYPE_CONTAINER:
                obj_field_int32_set(obj, OBJ_F_CONTAINER_NOTIFY_NPC, atoi(value));
                break;
            }
        } else if (strcmpi(key, "magic_hit_adjust") == 0) {
            sub_411880(&str, value);
            obj_field_int32_set(obj, OBJ_F_WEAPON_MAGIC_HIT_ADJ, atoi(value));
        } else if (strcmpi(key, "obj_flag") == 0) {
            flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
            sub_411880(&str, value);
            if (value[0] == '!') {
                for (index = 0; index < OF_COUNT; index++) {
                    if (strcmpi(&(value[1]), obj_flags_of[index]) == 0) {
                        flags &= ~(1 << index);
                        break;
                    }
                }
            } else {
                for (index = 0; index < OF_COUNT; index++) {
                    if (strcmpi(value, obj_flags_of[index]) == 0) {
                        flags |= 1 << index;
                        break;
                    }
                }
            }
            obj_field_int32_set(obj, OBJ_F_FLAGS, flags);
        } else if (strcmpi(key, "portal_flag") == 0) {
            flags = obj_field_int32_get(obj, OBJ_F_PORTAL_FLAGS);
            sub_411880(&str, value);
            if (value[0] == '!') {
                for (index = 0; index < OPF_COUNT; index++) {
                    if (strcmpi(&(value[1]), obj_flags_opf[index]) == 0) {
                        flags &= ~(1 << index);
                        break;
                    }
                }
            } else {
                for (index = 0; index < OPF_COUNT; index++) {
                    if (strcmpi(value, obj_flags_opf[index]) == 0) {
                        flags |= 1 << index;
                        break;
                    }
                }
            }
            obj_field_int32_set(obj, OBJ_F_PORTAL_FLAGS, flags);
        } else if (strcmpi(key, "item_flag") == 0) {
            flags = obj_field_int32_get(obj, OBJ_F_ITEM_FLAGS);
            sub_411880(&str, value);
            if (value[0] == '!') {
                for (index = 0; index < OIF_COUNT; index++) {
                    if (strcmpi(&(value[1]), obj_flags_oif[index]) == 0) {
                        flags &= ~(1 << index);
                        break;
                    }
                }
            } else {
                for (index = 0; index < OIF_COUNT; index++) {
                    if (strcmpi(value, obj_flags_oif[index]) == 0) {
                        flags |= 1 << index;
                        break;
                    }
                }
            }
            obj_field_int32_set(obj, OBJ_F_ITEM_FLAGS, flags);
        } else if (strcmpi(key, "critter_flag") == 0) {
            flags = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS);
            sub_411880(&str, value);
            if (value[0] == '!') {
                for (index = 0; index < OCF_COUNT; index++) {
                    if (strcmpi(&(value[1]), obj_flags_ocf[index]) == 0) {
                        flags &= ~(1 << index);
                        break;
                    }
                }
            } else {
                for (index = 0; index < OCF_COUNT; index++) {
                    if (strcmpi(value, obj_flags_ocf[index]) == 0) {
                        flags |= 1 << index;
                        break;
                    }
                }
            }
            obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS, flags);
        } else if (strcmpi(key, "npc_flag") == 0) {
            flags = obj_field_int32_get(obj, OBJ_F_NPC_FLAGS);
            sub_411880(&str, value);
            if (value[0] == '!') {
                for (index = 0; index < ONF_COUNT; index++) {
                    if (strcmpi(&(value[1]), obj_flags_onf[index]) == 0) {
                        flags &= ~(1 << index);
                        break;
                    }
                }
            } else {
                for (index = 0; index < ONF_COUNT; index++) {
                    if (strcmpi(value, obj_flags_onf[index]) == 0) {
                        flags |= 1 << index;
                        break;
                    }
                }
            }
            obj_field_int32_set(obj, OBJ_F_NPC_FLAGS, flags);
        } else if (strcmpi(key, "training") == 0) {
            int training;

            sub_411880(&str, value);
            for (training = 0; training < TRAINING_COUNT; training++) {
                if (strcmpi(value, off_5B7034[training]) == 0) {
                    sub_411880(&str, value);

                    for (index = 0; index < BASIC_SKILL_COUNT; index++) {
                        if (strcmpi(value, off_5B6FF4[index]) == 0) {
                            basic_skill_set_training(obj, index, training);
                        }
                    }

                    for (index = 0; index < TECH_SKILL_COUNT; index++) {
                        if (strcmpi(value, off_5B7024[index]) == 0) {
                            tech_skill_set_training(obj, index, training);
                        }
                    }
                }
            }
        } else if (strcmpi(key, "script") == 0) {
            Script scr;
            int sap;

            sub_411880(&str, value);
            sap = atoi(value);

            sub_411880(&str, value);
            scr.num = atoi(value);

            if (sub_44BCC0(&scr)) {
                obj_arrayfield_script_set(obj, OBJ_F_SCRIPTS_IDX, sap, &scr);
            }
        } else if (strcmpi(key, "daystand") == 0) {
            int64_t x;
            int64_t y;

            sub_411880(&str, value);
            x = _atoi64(value);

            sub_411880(&str, value);
            y = _atoi64(value);

            obj_field_int64_set(obj, OBJ_F_NPC_STANDPOINT_DAY, LOCATION_MAKE(x, y));
        } else if (strcmpi(key, "nightstand") == 0) {
            int64_t x;
            int64_t y;

            sub_411880(&str, value);
            x = _atoi64(value);

            sub_411880(&str, value);
            y = _atoi64(value);

            obj_field_int64_set(obj, OBJ_F_NPC_STANDPOINT_NIGHT, LOCATION_MAKE(x, y));
        } else if (strcmpi(key, "magic_tech_complexity") == 0) {
            sub_411880(&str, value);
            obj_field_int32_set(obj, OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY, atoi(value));
        } else if (strcmpi(key, "level_scheme") == 0) {
            sub_411880(&str, value);
            obj_field_int32_set(obj, OBJ_F_CRITTER_AUTO_LEVEL_SCHEME, atoi(value));
        } else if (strcmpi(key, "faction") == 0) {
            sub_411880(&str, value);
            obj_field_int32_set(obj, OBJ_F_NPC_FACTION, atoi(value));
        } else {
            tig_debug_printf("map_apply_obj_patch: Unknown attribute %s.\n", str);
            return;
        }
    }
}

// 0x412380
void map_enable_gender_check()
{
    map_gender_check_enabled = true;
}

// 0x412390
void map_gender_check()
{
    char name[1000];
    int64_t location;
    int64_t obj;
    int iter;

    if (!map_gender_check_enabled) {
        return;
    }

    if (obj_inst_first(&obj, &iter)) {
        do {
            if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
                const int description = obj_field_int32_get(obj, OBJ_F_DESCRIPTION);
                if (description >= DESCRIPTION_FIRST_CRITTER_NAME && description <= DESCRIPTION_LAST_CRITTER_NAME) {
                    const int actual_gender = stat_base_get(obj, STAT_GENDER);
                    const int expected_gender = description % GENDER_COUNT;
                    if (actual_gender == GENDER_MALE && expected_gender == GENDER_FEMALE) {
                        sub_441B60(obj, obj, name);

                        location = obj_field_int64_get(obj, OBJ_F_LOCATION);
                        tig_debug_printf("Male character with female description: %s at location %I64d %I64d\n",
                            name,
                            location);
                    } else if (actual_gender == GENDER_FEMALE && expected_gender == GENDER_FEMALE) {
                        // FIXME: Wrong condition above.
                        sub_441B60(obj, obj, name);

                        location = obj_field_int64_get(obj, OBJ_F_LOCATION);
                        tig_debug_printf("Female character with male description: %s at location %I64d %I64d\n",
                            name,
                            location);
                    }
                }

                const int unknown_description = obj_field_int32_get(obj, OBJ_F_CRITTER_DESCRIPTION_UNKNOWN);
                if (unknown_description >= DESCRIPTION_FIRST_CRITTER_NAME && unknown_description <= DESCRIPTION_LAST_CRITTER_NAME) {
                    const int actual_gender = stat_base_get(obj, STAT_GENDER);
                    const int expected_gender = unknown_description % GENDER_COUNT;
                    if (actual_gender == GENDER_MALE && expected_gender == GENDER_FEMALE) {
                        sub_441B60(obj, obj, name);

                        location = obj_field_int64_get(obj, OBJ_F_LOCATION);
                        tig_debug_printf("Male character with female unknown description: %s at location %I64d %I64d\n",
                            name,
                            location);
                    } else if (actual_gender == GENDER_FEMALE && expected_gender == GENDER_FEMALE) {
                        // FIXME: Wrong condition above.
                        sub_441B60(obj, obj, name);

                        location = obj_field_int64_get(obj, OBJ_F_LOCATION);
                        tig_debug_printf("Female character with male unknown description: %s at location %I64d %I64d\n",
                            name,
                            location);
                    }
                }
            }
        } while (obj_inst_next(&obj, &iter));
    }
}

// 0x4125C0
bool map_list_info_load()
{
    MesFileEntry mes_file_entry;

    if (!mes_load("Rules\\MapList.mes", &dword_5D11D8)) {
        return false;
    }

    tig_str_parse_set_separator(',');

    mes_file_entry.num = 5000;
    while (mes_search(dword_5D11D8, &mes_file_entry)) {
        char* str = mes_file_entry.str;
        tig_str_parse_str_value(&str, map_list_info[map_list_info_count].name);

        // NOTE: This check is silly. When string it long enough it will easly
        // overrun `MAP_NAME_LENGTH` and crash before this check
        if (strlen(map_list_info[map_list_info_count].name) >= MAP_NAME_LENGTH) {
            tig_debug_println("ERROR: Map name is too long");
            mes_unload(dword_5D11D8);
            return false;
        }

        tig_str_parse_value_64(&str, &(map_list_info[map_list_info_count].x));
        tig_str_parse_value_64(&str, &(map_list_info[map_list_info_count].y));

        if (str != NULL) {
            int type;
            if (tig_str_match_named_str_to_list(&str, "Type:", off_59F058, MAP_TYPE_COUNT, &type)) {
                dword_5D1214[type] = map_list_info_count + 1;
            }
        }

        map_list_info[map_list_info_count].worldmap = -1;
        if (str != NULL) {
            int worldmap;
            if (tig_str_parse_named_value(&str, "WorldMap:", &worldmap)) {
                map_list_info[map_list_info_count].worldmap = worldmap;
            }
        }

        map_list_info[map_list_info_count].area = 0;
        if (str != NULL) {
            int area;
            if (tig_str_parse_named_value(&str, "Area:", &area)) {
                map_list_info[map_list_info_count].area = area;
            }
        }

        if (++map_list_info_count >= MAP_LIST_CAPACITY) {
            break;
        }

        mes_file_entry.num++;
    }

    mes_unload(dword_5D11D8);
    return true;
}

// 0x4127A0
int map_list_info_find(const char* name)
{
    for (int index = 0; index < map_list_info_count; index++) {
        if (_strcmpi(map_list_info[index].name, name) == 0) {
            return index;
        }
    }

    return -1;
}

// 0x412830
bool map_list_info_set(int index, const char* name, int64_t x, int64_t y, bool is_start_map, int worldmap, int area)
{
    strcpy(map_list_info[index].name, name);
    map_list_info[index].x = x;
    map_list_info[index].y = y;
    map_list_info[index].worldmap = worldmap;
    map_list_info[index].area = area;

    if (is_start_map) {
        dword_5D1214[MAP_TYPE_START_MAP] = index + 1;
    } else {
        if (dword_5D1214[MAP_TYPE_START_MAP] == index + 1) {
            dword_5D1214[MAP_TYPE_START_MAP] = 0;
        }
    }

    return true;
}

// 0x412BD0
bool map_list_info_add(const char* name, int64_t x, int64_t y, bool is_start_map)
{
    int index;

    index = map_list_info_find(name);
    if (index != -1) {
        return map_list_info_set(index, name, x, y, is_start_map, map_list_info[index].worldmap, map_list_info[index].area);
    }

    index = map_list_info_find(off_59F3DC);
    if (index != -1) {
        return map_list_info_set(index, name, x, y, is_start_map, -1, 0);
    }

    return map_list_info_add_internal(name, x, y, is_start_map);
}

// 0x412C90
bool map_list_info_add_internal(const char* name, int64_t x, int64_t y, bool is_start_map)
{
    if (map_list_info_count < MAP_LIST_CAPACITY) {
        return map_list_info_set(map_list_info_count++, name, x, y, is_start_map, -1, 0);
    }

    return false;
}
