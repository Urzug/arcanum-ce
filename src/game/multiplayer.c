#include "game/multiplayer.h"

#include <stdio.h>

#include "game/anim.h"
#include "game/background.h"
#include "game/combat.h"
#include "game/critter.h"
#include "game/descriptions.h"
#include "game/effect.h"
#include "game/gamelib.h"
#include "game/item.h"
#include "game/level.h"
#include "game/light_scheme.h"
#include "game/magictech.h"
#include "game/map.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/obj_private.h"
#include "game/object.h"
#include "game/party.h"
#include "game/player.h"
#include "game/portrait.h"
#include "game/proto.h"
#include "game/rumor.h"
#include "game/skill.h"
#include "game/stat.h"
#include "game/tb.h"
#include "game/tech.h"
#include "game/tf.h"
#include "game/timeevent.h"
#include "game/trap.h"
#include "game/ui.h"

#define NUM_PLAYERS 8

#define BEGIN_SENTINEL 0x4ACEBABE
#define END_SENTINEL 0xBABE01CE

typedef struct MultiplayerObjectNode {
    /* 0000 */ ObjectID oid;
    /* 0018 */ struct MultiplayerObjectNode* next;
} MultiplayerObjectNode;

typedef struct MultiplayerPlayerSlot {
    /* 0000 */ unsigned int flags;
    /* 0004 */ int field_4;
    /* 0008 */ ObjectID field_8;
    /* 0020 */ int field_20;
    /* 0024 */ int field_24;
    /* 0028 */ int field_28;
    /* 002C */ int field_2C;
    /* 0030 */ int field_30;
    /* 0034 */ int field_34;
    /* 0038 */ ObjectID field_38;
} MultiplayerPlayerSlot;

// Serializeable.
static_assert(sizeof(MultiplayerPlayerSlot) == 0x50, "wrong size");

typedef struct MultiplayerMessageNode {
    /* 0000 */ int field_0;
    /* 0004 */ int field_4;
    /* 0008 */ int field_8;
    /* 000C */ int field_C;
    /* 0010 */ int field_10;
    /* 0014 */ int field_14;
    /* 0018 */ int field_18;
    /* 001C */ int field_1C;
    /* 0020 */ int field_20;
    /* 0024 */ int field_24;
    /* 0028 */ int field_28;
    /* 002C */ int field_2C;
    /* 0030 */ int field_30;
    /* 0034 */ int field_34;
    /* 0038 */ int field_38;
    /* 003C */ int field_3C;
    /* 0040 */ int field_40;
    /* 0044 */ int field_44;
    /* 0048 */ int field_48;
    /* 004C */ int field_4C;
    /* 0050 */ int field_50;
    /* 0054 */ int field_54;
    /* 0058 */ int field_58;
    /* 005C */ int field_5C;
    /* 0060 */ int field_60;
    /* 0064 */ int field_64;
    /* 0068 */ int field_68;
    /* 006C */ int field_6C;
    /* 0070 */ int field_70;
    /* 0074 */ int field_74;
    /* 0078 */ int field_78;
    /* 007C */ int field_7C;
    /* 0080 */ int field_80;
    /* 0084 */ int field_84;
    /* 0088 */ int field_88;
    /* 008C */ int field_8C;
    /* 0090 */ int field_90;
    /* 0094 */ int field_94;
    /* 0098 */ int field_98;
    /* 009C */ int field_9C;
    /* 00A0 */ int field_A0;
    /* 00A4 */ int field_A4;
    /* 00A8 */ int field_A8;
    /* 00AC */ int field_AC;
    /* 00B0 */ int field_B0;
    /* 00B4 */ int field_B4;
    /* 00B8 */ int field_B8;
    /* 00BC */ int field_BC;
    /* 00C0 */ int field_C0;
    /* 00C4 */ int field_C4;
    /* 00C8 */ int field_C8;
    /* 00CC */ int field_CC;
    /* 00D0 */ int field_D0;
    /* 00D4 */ int field_D4;
    /* 00D8 */ int field_D8;
    /* 00DC */ int field_DC;
    /* 00E0 */ int field_E0;
    /* 00E4 */ int field_E4;
    /* 00E8 */ int field_E8;
    /* 00EC */ int field_EC;
    /* 00F0 */ int field_F0;
    /* 00F4 */ int field_F4;
    /* 00F8 */ int field_F8;
    /* 00FC */ int field_FC;
    /* 0100 */ int field_100;
    /* 0104 */ int field_104;
    /* 0108 */ int field_108;
    /* 010C */ int field_10C;
    /* 0110 */ int field_110;
    /* 0114 */ int field_114;
    /* 0118 */ int field_118;
    /* 011C */ int field_11C;
    /* 0120 */ int field_120;
    /* 0124 */ int field_124;
    /* 0128 */ int field_128;
    /* 012C */ int field_12C;
    /* 0130 */ int field_130;
    /* 0134 */ int field_134;
    /* 0138 */ int field_138;
    /* 013C */ int field_13C;
    /* 0140 */ int field_140;
    /* 0144 */ int field_144;
    /* 0148 */ int field_148;
    /* 014C */ int field_14C;
    /* 0150 */ int field_150;
    /* 0154 */ int field_154;
    /* 0158 */ int field_158;
    /* 015C */ int field_15C;
    /* 0160 */ int field_160;
    /* 0164 */ int field_164;
    /* 0168 */ int field_168;
    /* 016C */ int field_16C;
    /* 0170 */ int field_170;
    /* 0174 */ int field_174;
    /* 0178 */ int field_178;
    /* 017C */ int field_17C;
    /* 0180 */ int field_180;
    /* 0184 */ int field_184;
    /* 0188 */ int field_188;
    /* 018C */ int field_18C;
    /* 0190 */ int field_190;
    /* 0194 */ int field_194;
    /* 0198 */ int field_198;
    /* 019C */ int field_19C;
    /* 01A0 */ int field_1A0;
    /* 01A4 */ int field_1A4;
    /* 01A8 */ struct MultiplayerMessageNode* next;
    /* 01AC */ int field_1AC;
} MultiplayerMessageNode;

typedef struct MultiplayerObjectLock {
    /* 0000 */ ObjectID field_0;
    /* 0018 */ ObjectID field_18;
    /* 0030 */ int field_30;
    /* 0034 */ int field_34;
    /* 0038 */ struct MultiplayerObjectLock* next;
    /* 003C */ int field_3C;
} MultiplayerObjectLock;

typedef struct MultiplayerLevelSchemeInfo {
    /* 0000 */ ObjectID oid;
    /* 0018 */ char rule[MAX_STRING];
    /* 07E8 */ char name[MAX_STRING];
} MultiplayerLevelSchemeInfo;

typedef struct MultiplayerCharDataHeader {
    /* 0000 */ ObjectID oid;
    /* 0018 */ int level;
    /* 001C */ int size;
} MultiplayerCharDataHeader;

typedef struct MultiplayerAsyncCallback {
    /* 0000 */ bool (*success_func)(void*);
    /* 0004 */ void* success_info;
    /* 0008 */ bool (*failure_func)(void*);
    /* 000C */ void* failure_info;
} MultiplayerAsyncCallback;

static void multiplayer_player_slot_init(MultiplayerPlayerSlot* a1);
static void multiplayer_start_play(PlayerCreateInfo* player_create_info);
static bool multiplayer_timeevent_is_xfer(TimeEvent* timeevent);
static void multiplayer_handle_message(void* msg);
static void multiplayer_teleport_obj(int64_t obj, int64_t location, int dx, int dy);
static bool multiplayer_get_player_obj(int player, int64_t* obj_ptr);
static void multiplayer_send_player_list();
static bool multiplayer_validate_message(void* msg);
static void multiplayer_send_loading_state(int a1);
static bool multiplayer_handle_network_event(int type, int client_id, void* data, int size);
static void multiplayer_update_game_state();
static void multiplayer_notify_player_lagging(int64_t obj);
static void multiplayer_notify_player_recovered(int64_t obj);
static void multiplayer_reset_player_slot(int player);
static void multiplayer_queue_message(MultiplayerMessageNode* a1);
static void multiplayer_clear_object_locks();
static bool multiplayer_acquire_object_lock(ObjectID a, ObjectID b, int player);
static void multiplayer_add_object_lock(ObjectID a1, ObjectID a2, int a3);
static MultiplayerObjectLock* multiplayer_find_object_lock(ObjectID oid);
static void multiplayer_release_object_lock(ObjectID oid);
static void multiplayer_remove_object_lock(ObjectID oid);
static void multiplayer_send_player_files(int player);
static void multiplayer_send_local_player_files_to_host();
static bool save_char(const char* path, int64_t obj);
static bool load_char(const char* path, int64_t* obj_ptr);
static bool multiplayer_has_char_data(int player);
static void multiplayer_level_scheme_set(int64_t obj, const char* rule, const char* name);
static int multiplayer_level_scheme_get(int64_t obj, char* rule, char* name);
static void multiplayer_init_hidden_items();
static bool multiplayer_add_hidden_item(int client_id, int64_t item_obj);
static bool multiplayer_remove_hidden_item(int client_id);
static void multiplayer_init_trade_list();
static void multiplayer_anim_ping_handler();
static void multiplayer_magictech_ping_handler();
static void multiplayer_auto_equip_char(int64_t obj);
static int multiplayer_auto_equip_give_money(int64_t obj, mes_file_handle_t mes_file);
static int multiplayer_auto_equip_give_background_items(int64_t obj);
static int multiplayer_auto_equip_give_skill_items(int64_t obj, mes_file_handle_t mes_file);
static int multiplayer_auto_equip_give_items_from_mes(int64_t obj, mes_file_handle_t mes_file, int num);
static int multiplayer_auto_equip_give_weapon_items(int64_t obj, mes_file_handle_t mes_file);
static void multiplayer_auto_equip_give_armor(int64_t obj, mes_file_handle_t mes_file);
static int multiplayer_auto_equip_check_has_items(int64_t obj, char* str);
static int multiplayer_auto_equip_give_items_from_string(int64_t obj, char* str);
static bool multiplayer_auto_equip_give_schematic_items(int64_t obj);
static void multiplayer_reset_critter_state(int64_t obj);
static bool multiplayer_strip_guid_from_filename(const char* a1, char* a2);

// 0x5B3FD8
static int g_multiplayer_current_difficulty = 10;

// 0x5B3FEC
static tig_button_handle_t g_multiplayer_disconnect_button_handle = TIG_BUTTON_HANDLE_INVALID;

static const struct {
    const char* name;
    bool (*save_func)(TigFile* stream);
    bool (*load_func)(GameLoadInfo* load_info);
    void (*ping_func)();
} g_multiplayer_save_modules[8] = {
    { "Anim", anim_save, anim_load, multiplayer_anim_ping_handler },
    { "MagicTech", magictech_post_save, magictech_post_load, multiplayer_magictech_ping_handler },
    { "Trap", mp_save, mp_load, NULL },
    { "Quest", quest_save, quest_load, NULL },
    { "Rumor", rumor_save, rumor_load, NULL },
    { "Party", party_save, party_load, NULL },
    { "LightScheme", light_scheme_save, light_scheme_load, NULL },
    { "Script", script_save, script_load, NULL },
};

// 0x5B4070
static int g_multiplayer_timeevent_filter_player_idx = -1;

// 0x5B40D8
static int g_multiplayer_player_find_iterator = -1;

// 0x5E8838
static char g_multiplayer_module_path_stripped[TIG_MAX_PATH];

// 0x5E8940
static TigIdxTable g_multiplayer_async_callbacks;

// 0x5E8AD0
static MultiplayerPlayerSlot g_multiplayer_player_slots[NUM_PLAYERS];

// 0x5E8D50
static char byte_5E8D50[40];

// 0x5E8D78
static int64_t g_multiplayer_trade_partners[NUM_PLAYERS];

// 0x5E8DB8
static char byte_5E8DB8[80];

// 0x5E8E08
static MultiplayerLevelSchemeInfo multiplayer_level_scheme_tbl[NUM_PLAYERS];

// 0x5F0BE8
static mes_file_handle_t g_multiplayer_mes_file;

// 0x5F0BEC
static char g_multiplayer_module_path_guid[TIG_MAX_PATH];

// 0x5F0BC8
static MultiplayerCharDataHeader* g_multiplayer_char_data_slots[NUM_PLAYERS];

// 0x5F0D18
static char byte_5F0D18[128];

// 0x5F0D98
static TigGuid g_multiplayer_current_module_guid;

// 0x5F0DE0
static int g_multiplayer_join_request_count;

// 0x5F0DE4
static int64_t* g_multiplayer_pregen_char_handles;

// 0x5F0DE8
static int g_multiplayer_pregen_char_count;

// 0x5F0DEC
static MultiplayerObjectNode* g_multiplayer_save_obj_list;

// 0x5F0DF4
static int g_multiplayer_level_scheme_next_idx;

// 0x5F0DF8
static MultiplayerDisconnectCallback* g_multiplayer_disconnect_callback;

// 0x5F0DFC
static MultiplayerMessageNode* g_multiplayer_message_queue_head;

// 0x5F0E00
static bool g_multiplayer_session_active;

// 0x5F0E04
static void (*g_multiplayer_game_update_override)();

// 0x5F0E08
static MultiplayerClientJoinedCallback* g_multiplayer_client_joined_callback;

// 0x5F0E0C
static int g_multiplayer_lock_count;

// 0x5F0E10
static int g_multiplayer_reset_lock_count;

// 0x5F0E14
static bool g_multiplayer_disconnect_pending;

// 0x5F0E18
static int g_multiplayer_async_callback_next_idx;

// 0x5F0E1C
static MultiplayerObjectLock* g_multiplayer_object_lock_list;

// 0x5F0E20
static int64_t g_multiplayer_hidden_items[NUM_PLAYERS];

// 0x49C670
bool multiplayer_init(GameInitInfo* init_info)
{
    TigFileInfo file_info;
    int index;

    (void)init_info;

    if (tig_file_exists("Players", &file_info)) {
        if ((file_info.attributes & TIG_FILE_ATTRIBUTE_SUBDIR) == 0) {
            tig_debug_printf("MP: init: ERROR: players folder (%s) could not be made (already a file).\n", "Players");
            return false;
        }
    } else {
        tig_file_mkdir_ex("Players");
    }

    g_multiplayer_disconnect_pending = false;

    if (!tig_net_local_client_set_name("Player")) {
        return false;
    }

    if (!tig_net_local_server_set_max_players(NUM_PLAYERS)) {
        return false;
    }

    if (!tig_net_local_server_set_description("Description")) {
        return false;
    }

    for (index = 0; index < NUM_PLAYERS; index++) {
        multiplayer_player_slot_init(&(g_multiplayer_player_slots[index]));
    }

    mes_load("mes\\MultiPlayer.mes", &g_multiplayer_mes_file);
    tig_idxtable_init(&g_multiplayer_async_callbacks, sizeof(MultiplayerAsyncCallback));
    g_multiplayer_save_obj_list = NULL;
    memset(g_multiplayer_char_data_slots, 0, sizeof(g_multiplayer_char_data_slots));
    multiplayer_init_hidden_items();
    multiplayer_init_trade_list();
    multiplayer_clear_object_locks();

    return true;
}

// 0x49C780
void multiplayer_exit()
{
    MultiplayerObjectNode* node;
    int index;

    if (g_multiplayer_session_active) {
        multiplayer_end();
    }

    while (g_multiplayer_save_obj_list != NULL) {
        node = g_multiplayer_save_obj_list;
        g_multiplayer_save_obj_list = g_multiplayer_save_obj_list->next;
        FREE(node);
    }

    for (index = 0; index < NUM_PLAYERS; index++) {
        if (g_multiplayer_char_data_slots[index] != NULL) {
            FREE(g_multiplayer_char_data_slots[index]);
            g_multiplayer_char_data_slots[index] = NULL;
        }
    }

    tig_idxtable_exit(&g_multiplayer_async_callbacks);
    mes_unload(g_multiplayer_mes_file);

    if (g_multiplayer_pregen_char_handles) {
        FREE(g_multiplayer_pregen_char_handles);
    }
}

// 0x49C820
void multiplayer_reset()
{
    MultiplayerObjectNode* node;
    int index;

    if (!g_multiplayer_reset_lock_count) {
        if (g_multiplayer_session_active) {
            multiplayer_end();
        }
        g_multiplayer_session_active = false;

        tig_idxtable_exit(&g_multiplayer_async_callbacks);
        tig_idxtable_init(&g_multiplayer_async_callbacks, 16);

        while (g_multiplayer_save_obj_list != NULL) {
            node = g_multiplayer_save_obj_list;
            g_multiplayer_save_obj_list = g_multiplayer_save_obj_list->next;
            FREE(node);
        }

        tig_net_local_server_set_name("Arcanum");
        tig_net_local_client_set_name("Player");
        tig_net_local_server_set_max_players(NUM_PLAYERS);
        tig_net_local_server_set_description("Description");

        for (index = 0; index < NUM_PLAYERS; index++) {
            multiplayer_player_slot_init(&(g_multiplayer_player_slots[index]));
        }

        for (index = 0; index < NUM_PLAYERS; index++) {
            if (g_multiplayer_char_data_slots[index] != NULL) {
                FREE(g_multiplayer_char_data_slots[index]);
                g_multiplayer_char_data_slots[index] = NULL;
            }
        }

        if (g_multiplayer_pregen_char_handles != NULL) {
            multiplayer_clear_pregen_char_list(false);
            g_multiplayer_pregen_char_handles = NULL;
            g_multiplayer_pregen_char_count = 0;
        }

        multiplayer_init_hidden_items();
        multiplayer_init_trade_list();
        timeevent_clear_all_typed(TIMEEVENT_TYPE_MULTIPLAYER);
    }
}

// 0x49C930
bool multiplayer_save(TigFile* stream)
{
    unsigned int sentinel;
    MultiplayerObjectNode* node;
    int cnt;

    if (!tig_net_is_active()) {
        return true;
    }

    sentinel = BEGIN_SENTINEL;
    if (tig_file_fwrite(&sentinel, sizeof(sentinel), 1, stream) != 1) {
        return false;
    }

    cnt = 0;
    node = g_multiplayer_save_obj_list;
    while (node != NULL) {
        cnt++;
        node = node->next;
    }

    if (tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
        return false;
    }

    node = g_multiplayer_save_obj_list;
    while (node != NULL) {
        if (tig_file_fwrite(&(node->oid), sizeof(node->oid), 1, stream) != 1) {
            return false;
        }
        node = node->next;
    }

    if (tig_file_fwrite(g_multiplayer_player_slots, sizeof(*g_multiplayer_player_slots), NUM_PLAYERS, stream) != NUM_PLAYERS) {
        return false;
    }

    sentinel = END_SENTINEL;
    if (tig_file_fwrite(&sentinel, sizeof(sentinel), 1, stream) != 1) {
        return false;
    }

    return true;
}

// 0x49CA20
bool mutliplayer_load(GameLoadInfo* load_info)
{
    unsigned int sentinel;
    MultiplayerObjectNode* node;
    int cnt;
    ObjectID oid;

    if (!tig_net_is_active()) {
        return true;
    }

    if (tig_file_fread(&sentinel, sizeof(sentinel), 1, load_info->stream) != 1) {
        return false;
    }

    if (sentinel != BEGIN_SENTINEL) {
        return false;
    }

    if (tig_file_fread(&cnt, sizeof(cnt), 1, load_info->stream) != 1) {
        return false;
    }

    while (cnt != 0) {
        if (tig_file_fread(&oid, sizeof(oid), 1, load_info->stream) != 1) {
            return false;
        }

        node = (MultiplayerObjectNode*)MALLOC(sizeof(*node));
        node->oid = oid;
        node->next = g_multiplayer_save_obj_list;
        g_multiplayer_save_obj_list = node;

        cnt--;
    }

    if (tig_file_fread(g_multiplayer_player_slots, sizeof(*g_multiplayer_player_slots), NUM_PLAYERS, load_info->stream) != NUM_PLAYERS) {
        return false;
    }

    if (tig_file_fread(&sentinel, sizeof(sentinel), 1, load_info->stream) != 1) {
        return false;
    }

    if (sentinel != END_SENTINEL) {
        return false;
    }

    return true;
}

// 0x49CB50
bool multiplayer_mod_load()
{
    return true;
}

// 0x49CB60
void multiplayer_mod_unload()
{
    multiplayer_lock();
    multiplayer_clear_pregen_char_list(true);
    multiplayer_unlock();
}

// 0x49CB80
void multiplayer_player_slot_init(MultiplayerPlayerSlot* a1)
{
    memset(a1, 0, sizeof(*a1));
    a1->flags = 0;
    a1->field_20 = -1;
    a1->field_24 = 1;
    a1->field_28 = 0;
    a1->field_2C = 0;
    a1->field_30 = 0;
    a1->field_34 = 0;
    memset(&(a1->field_8), 0, sizeof(a1->field_8));
    memset(&(a1->field_38), 0, sizeof(a1->field_38));
    a1->field_38.type = OID_TYPE_NULL;
}

// 0x49CBD0
bool multiplayer_start()
{
    g_multiplayer_lock_count = 0;
    if (tig_net_start_client() != TIG_OK) {
        return false;
    }

    multiplayer_reset_player_slot(0);
    tig_net_on_message(multiplayer_handle_message);
    tig_net_on_message_validation(multiplayer_validate_message);
    tig_net_on_network_event(multiplayer_handle_network_event);
    g_multiplayer_session_active = false;

    return true;
}

// 0x49CC20
bool multiplayer_end()
{
    bool v1;

    multiplayer_reset_player_slot(0);
    tig_net_on_message(NULL);
    v1 = sub_5280F0();
    g_multiplayer_session_active = false;
    g_multiplayer_lock_count = 0;
    return !v1;
}

// 0x49CC50
void multiplayer_start_server()
{
    tig_net_start_server();
}

// 0x49CC70
bool multiplayer_load_module_and_start(const char* a1, const char* a2)
{
    const char* suffixes[3] = {
        ".mpc",
        ".bmp",
        "_b.bmp",
    };
    mes_file_handle_t mes_file;
    TigWindowModalDialogInfo modal_dialog_info;
    MesFileEntry mes_file_entry;
    char path[TIG_MAX_PATH];
    char str[512];
    int map;
    char* map_name;
    PlayerCreateInfo player_create_info;
    int idx;
    char pc_file_base_name[40];
    char src[TIG_MAX_PATH];
    char dst[TIG_MAX_PATH];

    TimeEventResetPause();
    tig_file_empty_directory(".\\data\\temp");

    modal_dialog_info.type = TIG_WINDOW_MODAL_DIALOG_TYPE_OK;
    modal_dialog_info.x = 237;
    modal_dialog_info.y = 232;
    modal_dialog_info.process = NULL;
    modal_dialog_info.redraw = gamelib_redraw;

    mes_load("mes\\MultiPlayer.mes", &mes_file);

    snprintf(path, sizeof(path), "%s\\%s.dat", ".\\Modules", a1);
    if (tig_file_exists(path, NULL)) {
        tig_file_repository_add(path);
        if (tig_file_repository_guid(path, &g_multiplayer_current_module_guid)) {
            if (multiplayer_strip_guid_from_filename(path, g_multiplayer_module_path_stripped)) {
                strncpy(g_multiplayer_module_path_guid, path, sizeof(g_multiplayer_module_path_guid));
            } else {
                strncpy(g_multiplayer_module_path_stripped, path, sizeof(g_multiplayer_module_path_stripped));
                snprintf(g_multiplayer_module_path_guid, sizeof(g_multiplayer_module_path_guid),
                    "%s\\%s-{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}.dat",
                    ".\\Modules",
                    a1,
                    (g_multiplayer_current_module_guid.data[0] << 24) | (g_multiplayer_current_module_guid.data[1] << 16) | (g_multiplayer_current_module_guid.data[2] << 8) | g_multiplayer_current_module_guid.data[3],
                    (g_multiplayer_current_module_guid.data[4] << 8) | g_multiplayer_current_module_guid.data[5],
                    (g_multiplayer_current_module_guid.data[6] << 8) | g_multiplayer_current_module_guid.data[7],
                    g_multiplayer_current_module_guid.data[8],
                    g_multiplayer_current_module_guid.data[9],
                    g_multiplayer_current_module_guid.data[10],
                    g_multiplayer_current_module_guid.data[11],
                    g_multiplayer_current_module_guid.data[12],
                    g_multiplayer_current_module_guid.data[13],
                    g_multiplayer_current_module_guid.data[14],
                    g_multiplayer_current_module_guid.data[15]);
            }
        }
        tig_file_repository_remove(path);
    } else {
        mes_file_entry.num = 900;
        mes_get_msg(mes_file, &mes_file_entry);
        snprintf(str, sizeof(str), mes_file_entry.str, a1);
        modal_dialog_info.text = str;
        tig_window_modal_dialog(&modal_dialog_info, NULL);
    }

    if (!gamelib_mod_load(a1) || !ui_gameuilib_mod_load()) {
        if (!gamelib_mod_load(gamelib_default_module_name_get()) || !ui_gameuilib_mod_load()) {
            tig_debug_printf("Multiplayer: Could not load module %s, or default module %s.\n",
                a1,
                gamelib_default_module_name_get());
            exit(EXIT_SUCCESS); // FIXME: Should be `EXIT_FAILURE`.
        }

        mes_file_entry.num = 901;
        mes_get_msg(mes_file, &mes_file_entry);
        snprintf(str, sizeof(str), mes_file_entry.str, a1);
        modal_dialog_info.text = str;
        tig_window_modal_dialog(&modal_dialog_info, NULL);
        mes_unload(mes_file);
        return false;
    }

    tig_net_local_server_set_description(a1);
    PlayerDestroyLocalPC();

    if (a2 != NULL) {
        if (gamelib_load(a2)) {
            gamelib_current_mode_name_set(a1);
        }
    } else {
        map = map_by_type(MAP_TYPE_START_MAP);
        if (map == 0) {
            mes_file_entry.num = 902;
            mes_get_msg(mes_file, &mes_file_entry);
            modal_dialog_info.text = mes_file_entry.str;
            tig_window_modal_dialog(&modal_dialog_info, NULL);
            mes_unload(mes_file);
            return false;
        }

        if (!map_get_name(map, &map_name)) {
            mes_unload(mes_file);
            return false;
        }

        multiplayer_map_open_by_name(map_name);
        sub_52A940();

        multiplayer_start_play(&player_create_info);
        sub_52A950();
        location_origin_set(obj_field_int64_get(player_create_info.obj, OBJ_F_LOCATION));

        objid_id_to_str(pc_file_base_name, obj_get_id(player_create_info.obj));
        sprintf(dst, "%s\\Players", ".\\data\\temp");
        if (!tig_file_is_directory(dst)) {
            tig_file_mkdir(dst);
        }

        for (idx = 0; idx < 3; idx++) {
            sprintf(path, "%s%s", pc_file_base_name, suffixes[idx]);
            sprintf(src, "Players\\%s", path);
            sprintf(dst, "%s\\Players\\%s", ".\\data\\temp", path);
            tig_file_copy(src, dst);
        }
    }

    sub_52B210();
    multiplayer_apply_settings();
    sub_5286E0();
    mes_unload(mes_file);

    return true;
}

// 0x49D100
void multiplayer_start_play(PlayerCreateInfo* player_create_info)
{
    int64_t loc;
    int poison;
    mes_file_handle_t mes_file;
    TigWindowModalDialogInfo modal_dialog_info;
    MesFileEntry mes_file_entry;
    DateTime datetime;
    TimeEvent timeevent;

    player_create_info_init(player_create_info);
    map_starting_loc_get(&loc);
    player_create_info->loc = location_make(1, 1);
    mes_load("mes\\MultiPlayer.mes", &mes_file);

    modal_dialog_info.type = TIG_WINDOW_MODAL_DIALOG_TYPE_OK;
    modal_dialog_info.x = 237;
    modal_dialog_info.y = 232;
    modal_dialog_info.process = NULL;
    modal_dialog_info.redraw = gamelib_redraw;

    if (multiplayer_has_char_data(0)) {
        if (!object_deserialize_from_memory(multiplayer_get_char_data_ptr(0), &(player_create_info->obj), player_create_info->loc)) {
            exit(EXIT_FAILURE);
        }

        g_multiplayer_player_slots[0].field_8 = obj_get_id(player_create_info->obj);
        PlayerSetLocalPC(player_create_info->obj);
        critter_fatigue_damage_set(player_create_info->obj, 0);
        object_hp_damage_set(player_create_info->obj, 0);
        object_flags_unset(player_create_info->obj, OF_OFF);

        poison = stat_base_get(player_create_info->obj, STAT_POISON_LEVEL);
        if (poison != 0) {
            stat_base_set(player_create_info->obj, STAT_POISON_LEVEL, poison);
        }
    } else {
        mes_file_entry.num = 903;
        mes_get_msg(mes_file, &mes_file_entry);

        modal_dialog_info.text = mes_file_entry.str;
        tig_window_modal_dialog(&modal_dialog_info, NULL);

        player_create_info->obj = OBJ_HANDLE_NULL;
        player_create_info->flags = PLAYER_CREATE_INFO_LOC;
        if (!player_obj_create_player(player_create_info)) {
            tig_debug_printf("MP: multiplayer_start_play could not create_player");
            exit(EXIT_FAILURE);
        }

        g_multiplayer_player_slots[0].field_8 = player_create_info->oid;
        obj_field_string_set(player_create_info->obj, OBJ_F_PC_PLAYER_NAME, tig_net_client_info_get_name(0));
    }

    multiplayer_reset_critter_state(player_create_info->obj);
    multiplayer_auto_equip_char(player_create_info->obj);
    item_drop_at_loc(player_create_info->obj, loc, 0, 0, false);

    datetime.days = 0;
    datetime.milliseconds = 0;
    timeevent.type = TIMEEVENT_TYPE_TELEPORTED;
    timeevent.params[0].object_value = player_create_info->obj;
    timeevent_add_delay(&timeevent, &datetime);

    mes_unload(mes_file);
}

// 0x49D320
bool multiplayer_timeevent_process(TimeEvent* timeevent)
{
    DateTime datetime;
    Packet64 pkt;
    char* map_name;
    int player;
    char str[40];
    char path[TIG_MAX_PATH];

    switch (timeevent->params[0].integer_value) {
    case 2:
        if (tig_net_xfer_count(timeevent->params[1].integer_value)) {
            DateTimeAddMilliseconds(&datetime, 50);
            timeevent_add_delay(timeevent, &datetime);
            return true;
        }

        g_multiplayer_timeevent_filter_player_idx = timeevent->params[1].integer_value;
        timeevent_clear_all_ex(TIMEEVENT_TYPE_MULTIPLAYER, multiplayer_timeevent_is_xfer);
        g_multiplayer_timeevent_filter_player_idx = -1;

        pkt.type = 64;
        pkt.map = map_current_map();
        map_get_name(map_current_map(), &map_name);
        strcpy(pkt.name, map_name);
        pkt.player = timeevent->params[1].integer_value;
        pkt.field_118 = 0;
        tig_net_send_app_all(&pkt, sizeof(pkt));
        return true;
    case 4:
        if (tig_net_xfer_count(timeevent->params[1].integer_value)) {
            DateTimeAddMilliseconds(&datetime, 50);
            timeevent_add_delay(timeevent, &datetime);
            return true;
        }

        for (player = 1; player < 8; player++) {
            if (tig_net_client_is_active(player)
                && !tig_net_client_is_waiting(player)
                && !tig_net_client_is_loading(player)
                && player != timeevent->params[1].integer_value) {
                multiplayer_send_player_files(player);
            }
        }

        return true;
    case 3:
        objid_id_to_str(str, obj_get_id(player_get_local_pc_obj()));
        snprintf(path, sizeof(path), "Players\\%s.mpc", str);
        anim_set_priority_level(player_get_local_pc_obj(), PRIORITY_HIGHEST, false, true);

        if (sub_460BB0()) {
            save_char(path, player_get_local_pc_obj());
        }

        g_multiplayer_disconnect_pending = 0;
        if (g_multiplayer_disconnect_callback != NULL) {
            g_multiplayer_disconnect_callback(g_multiplayer_disconnect_button_handle);
        }

        multiplayer_reset();
        g_multiplayer_disconnect_callback = NULL;

        return true;
    case 5:
        sub_52A9E0(timeevent->params[1].integer_value);
        return true;
    default:
        return true;
    }
}

// 0x49D570
bool multiplayer_timeevent_is_xfer(TimeEvent* timeevent)
{
    return timeevent->params[0].integer_value == 2
        && timeevent->params[1].integer_value == g_multiplayer_timeevent_filter_player_idx;
}

// 0x49D590
bool multiplayer_map_open_by_name(const char* name)
{
    char path[TIG_MAX_PATH];
    char save_path[TIG_MAX_PATH];

    sprintf(path, ".\\%s.dat", name);
    if (tig_file_exists(path, NULL)) {
        tig_file_repository_add(path);
    }

    sub_52A940();
    map_flush(0);

    if (!obj_validate_system(1)) {
        tig_debug_println("Object system validate failed pre-load in multiplayer_map_open_by_name.");
        tig_message_post_quit(0);
    }

    sprintf(path, "maps\\%s", name);
    sprintf(save_path, "%s\\maps\\%s", "Save\\Current", name);
    tig_debug_printf("MP: Loading Map: %s\n", path);
    if (!map_open(path, save_path, 1)) {
        return false;
    }

    sub_52A950();
    sub_4605C0();

    if (!obj_validate_system(1)) {
        tig_debug_println("Object system validate failed post-load in multiplayer_map_open_by_name.");
        tig_message_post_quit(0);
    }

    return true;
}

// 0x49D690
void multiplayer_handle_message(void* msg)
{
    // TODO: Incomplete.
}

// 0x4A1F30
void multiplayer_teleport_obj(int64_t obj, int64_t location, int dx, int dy)
{
    if (location != 0) {
        object_move(obj, location, dx, dy);
    }
}

// 0x4A1F60
bool multiplayer_get_player_obj(int player, int64_t* obj_ptr)
{
    if (player >= 0 && player < NUM_PLAYERS) {
        *obj_ptr = objp_perm_lookup(g_multiplayer_player_slots[player].field_8);
        return *obj_ptr != OBJ_HANDLE_NULL;
    }

    *obj_ptr = OBJ_HANDLE_NULL;
    return false;
}

// 0x4A1FC0
void multiplayer_send_player_list()
{
    PacketGamePlayerList pkt;
    int cnt;
    int index;

    if (tig_net_is_host()) {
        pkt.type = 2;

        cnt = tig_net_local_server_get_max_players();
        for (index = 0; index < cnt; index++) {
            pkt.oids[index] = g_multiplayer_player_slots[index].field_8;
        }

        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4A2020
bool multiplayer_validate_message(void* msg)
{
    int type;

    type = *(int*)msg;
    return type > 0 && type < 131;
}

// 0x4A2040
void multiplayer_send_loading_state(int a1)
{
    Packet67 pkt;

    pkt.type = 67;
    pkt.field_4 = a1;
    tig_net_send_app_all(&pkt, sizeof(pkt));
}

// 0x4A2070
bool multiplayer_handle_network_event(int type, int client_id, void* data, int size)
{
    // TODO: Incomplete.
}

// 0x4A2A30
void multiplayer_update_game_state()
{
    if (g_multiplayer_game_update_override != NULL) {
        g_multiplayer_game_update_override();
    } else {
        sub_4606E0();
    }
}

// 0x4A2A40
void multiplayer_notify_player_lagging(int64_t obj)
{
    PacketNotifyPlayerLagging pkt;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    anim_lag_icon_add(obj);

    if (tig_net_is_host()) {
        pkt.type = 68;
        follower_info_init(obj, &(pkt.field_8));
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4A2A90
void multiplayer_notify_player_recovered(int64_t obj)
{
    PacketNotifyPlayerRecovered pkt;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    anim_lag_icon_remove(obj);

    if (tig_net_is_host()) {
        pkt.type = 69;
        follower_info_init(obj, &(pkt.field_8));
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4A2AE0
void multiplayer_reset_player_slot(int player)
{
    multiplayer_player_slot_init(&g_multiplayer_player_slots[player]);
}

// 0x4A2B00
void multiplayer_set_client_joined_callback(MultiplayerClientJoinedCallback* func)
{
    g_multiplayer_client_joined_callback = func;
}

// 0x4A2B10
int multiplayer_find_slot_from_obj(int64_t obj)
{
    int player;
    int64_t player_obj;

    for (player = 0; player < NUM_PLAYERS; player++) {
        if (tig_net_client_is_active(player)
            && multiplayer_get_player_obj(player, &player_obj)
            && player_obj == obj) {
            return player;
        }
    }

    return -1;
}

// 0x4A2B60
int64_t multiplayer_get_player_obj_by_slot(int player)
{
    int64_t obj;

    if (tig_net_client_is_active(player)
        && multiplayer_get_player_obj(player, &obj)) {
        return obj;
    }

    return OBJ_HANDLE_NULL;
}

// 0x4A2BA0
bool multiplayer_is_locked()
{
    return tig_net_is_active() ? g_multiplayer_lock_count > 0 : true;
}

// 0x4A2BC0
void multiplayer_lock()
{
    g_multiplayer_lock_count++;
}

// 0x4A2BD0
void multiplayer_unlock()
{
    if (g_multiplayer_lock_count > 0) {
        g_multiplayer_lock_count--;
    }
}

// 0x4A2BE0
int64_t multiplayer_player_find_first()
{
    int64_t obj;

    g_multiplayer_player_find_iterator = -1;

    if (!tig_net_is_active()) {
        return player_get_local_pc_obj();
    }

    for (g_multiplayer_player_find_iterator = 0; g_multiplayer_player_find_iterator < NUM_PLAYERS; g_multiplayer_player_find_iterator++) {
        if (tig_net_client_is_active(g_multiplayer_player_find_iterator)
            && multiplayer_get_player_obj(g_multiplayer_player_find_iterator, &obj)) {
            g_multiplayer_player_find_iterator++;
            return obj;
        }
    }

    return OBJ_HANDLE_NULL;
}

// 0x4A2C60
int64_t multiplayer_player_find_next()
{
    int64_t obj;

    if (!tig_net_is_active()) {
        return OBJ_HANDLE_NULL;
    }

    for (; g_multiplayer_player_find_iterator < NUM_PLAYERS; g_multiplayer_player_find_iterator++) {
        if (tig_net_client_is_active(g_multiplayer_player_find_iterator)
            && multiplayer_get_player_obj(g_multiplayer_player_find_iterator, &obj)) {
            g_multiplayer_player_find_iterator++;
            return obj;
        }
    }

    return OBJ_HANDLE_NULL;
}

// 0x4A2CD0
void multiplayer_queue_message(MultiplayerMessageNode* a1)
{
    MultiplayerMessageNode* node;

    node = (MultiplayerMessageNode*)MALLOC(sizeof(*node));
    memcpy(node, a1, sizeof(*node));
    node->next = g_multiplayer_message_queue_head;
    g_multiplayer_message_queue_head = node;
}

// 0x4A2D00
void multiplayer_process_message_queue()
{
    // TODO: Incomplete.
}

// 0x4A2E90
void multiplayer_clear_object_locks()
{
    MultiplayerObjectLock* node;

    while (g_multiplayer_object_lock_list != NULL) {
        node = g_multiplayer_object_lock_list;
        g_multiplayer_object_lock_list = g_multiplayer_object_lock_list->next;
        FREE(node);
    }
}

// 0x4A2EC0
bool multiplayer_acquire_object_lock(ObjectID item_oid, ObjectID parent_oid, int player)
{
    int64_t item_obj;
    int64_t parent_obj;
    int64_t tmp_parent_obj;
    MultiplayerObjectLock* lock;

    item_obj = objp_perm_lookup(item_oid);
    sub_4F0690(parent_oid, &parent_obj);

    if (obj_type_is_item(obj_field_int32_get(item_obj, OBJ_F_TYPE))) {
        if (item_parent(item_obj, &tmp_parent_obj)
            && tmp_parent_obj != parent_obj) {
            return false;
        }

        if ((obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_NO_DISPLAY) != 0) {
            if (multiplayer_get_player_obj_by_slot(player) != parent_obj) {
                return false;
            }
        }
    }

    if ((obj_field_int32_get(item_obj, OBJ_F_FLAGS) & OF_MULTIPLAYER_LOCK) != 0) {
        lock = multiplayer_find_object_lock(item_oid);
        if (lock == NULL) {
            return false;
        }

        if (lock->field_30 != player) {
            return false;
        }

        lock->field_34++;
        return true;
    } else {
        object_flags_set(item_obj, OF_MULTIPLAYER_LOCK);
        multiplayer_add_object_lock(item_oid, parent_oid, player);
        return true;
    }
}

// 0x4A3030
void multiplayer_add_object_lock(ObjectID a1, ObjectID a2, int a3)
{
    MultiplayerObjectLock* node;

    node = (MultiplayerObjectLock*)MALLOC(sizeof(*node));
    node->field_0 = a1;
    node->field_18 = a2;
    node->field_30 = a3;
    node->field_34 = 1;
    node->next = g_multiplayer_object_lock_list;
    g_multiplayer_object_lock_list = node;
}

// 0x4A3080
MultiplayerObjectLock* multiplayer_find_object_lock(ObjectID oid)
{
    MultiplayerObjectLock* node;

    node = g_multiplayer_object_lock_list;
    while (node != NULL) {
        if (objid_is_equal(node->field_0, oid)) {
            return node;
        }
        node = node->next;
    }

    return NULL;
}

// 0x4A30D0
void multiplayer_release_object_lock(ObjectID oid)
{
    int64_t obj;
    MultiplayerObjectLock* node;

    sub_4F0690(oid, &obj);
    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_MULTIPLAYER_LOCK) != 0) {
        node = multiplayer_find_object_lock(oid);
        if (node != NULL) {
            node->field_34--;
            if (node->field_34 == 0) {
                object_flags_unset(obj, OF_MULTIPLAYER_LOCK);
                multiplayer_remove_object_lock(oid);
            }
        }
    }
}

// 0x4A3170
void multiplayer_remove_object_lock(ObjectID oid)
{
    MultiplayerObjectLock* node;
    MultiplayerObjectLock** parent_ptr;

    if (g_multiplayer_object_lock_list != NULL) {
        // NOTE: Probably can be joined into loop.
        if (objid_is_equal(g_multiplayer_object_lock_list->field_0, oid)) {
            node = g_multiplayer_object_lock_list;
            g_multiplayer_object_lock_list = g_multiplayer_object_lock_list->next;
            FREE(node);
        } else {
            node = g_multiplayer_object_lock_list;
            parent_ptr = &(g_multiplayer_object_lock_list->next);
            while (node->next != NULL) {
                if (objid_is_equal(node->field_0, oid)) {
                    *parent_ptr = node->next;
                    FREE(node);
                    break;
                }
                node = node->next;
                parent_ptr = &((*parent_ptr)->next);
            }
        }
    }
}

// 0x4A3230
void multiplayer_request_object_lock(ObjectID oid, bool (*success_func)(void*), void* success_info, bool (*failure_func)(void*), void* failure_info)
{
    if (tig_net_is_active()) {
        int64_t item_obj;
        int64_t parent_obj;
        ObjectID parent_oid;

        sub_4F0690(oid, &item_obj);
        item_parent(item_obj, &parent_obj);
        obj_get_oid(parent_obj, &parent_oid);
        if (tig_net_is_host()) {
            if (multiplayer_acquire_object_lock(oid, parent_oid, 0)) {
                if (success_func != NULL) {
                    success_func(success_info);
                }

                multiplayer_release_object_lock(oid);
            } else {
                if (failure_func != NULL) {
                    failure_func(failure_info);
                }
            }
        } else {
            MultiplayerAsyncCallback entry;
            Packet80 pkt;

            entry.success_func = success_func;
            entry.success_info = success_info;
            entry.failure_func = failure_func;
            entry.failure_info = failure_info;
            tig_idxtable_set(&g_multiplayer_async_callbacks, g_multiplayer_async_callback_next_idx, &entry);

            pkt.type = 80;
            pkt.item_oid = oid;
            pkt.parent_oid = parent_oid;
            pkt.idx = g_multiplayer_async_callback_next_idx;
            pkt.field_3C = 1;
            tig_net_send_app_all(&pkt, sizeof(pkt));

            g_multiplayer_async_callback_next_idx++;
        }
    } else {
        if (success_func != NULL) {
            success_func(success_info);
        }
    }
}

// 0x4A33F0
void multiplayer_send_map_data_to_player(int a1, int a2)
{
    int player;
    int64_t obj;
    char* map_name;
    char path[TIG_MAX_PATH];
    TigFile* stream;
    int func_idx;
    TigFileList file_list;
    unsigned int file_idx;
    char src_path[TIG_MAX_PATH];
    char dst_path[TIG_MAX_PATH];
    TimeEvent timeevent;
    DateTime datetime;

    sub_52A940();
    map_flush(0);

    for (player = 0; player < 8; player++) {
        if (tig_net_client_is_active(player)
            && g_multiplayer_player_slots[player].field_8.type != OID_TYPE_NULL) {
            sub_4F0690(g_multiplayer_player_slots[player].field_8, &obj);
            map_precache_sectors(obj_field_int64_get(obj, OBJ_F_LOCATION));
        }
    }

    sub_52A950();

    map_get_name(map_current_map(), &map_name);
    sprintf(path, "save\\current\\maps\\%s\\xferdata.mp", map_name);

    stream = tig_file_fopen(path, "wb");
    if (stream != NULL) {
        for (func_idx = 0; func_idx < 8; func_idx++) {
            if (g_multiplayer_save_modules[func_idx].save_func != NULL) {
                g_multiplayer_save_modules[func_idx].save_func(stream);
            }
        }
        tig_file_fclose(stream);

        sprintf(path, "%s\\maps\\%s\\*.*", "Save\\Current", map_name);
        tig_file_list_create(&file_list, path);

        for (file_idx = 0; file_idx < file_list.count; file_idx++) {
            if (strcmp(file_list.entries[file_idx].path, ".") != 0
                && strcmp(file_list.entries[file_idx].path, "..") != 0) {
                sprintf(src_path, "%s\\maps\\%s\\%s", "Save\\Current", map_name, file_list.entries[file_idx].path);
                sprintf(dst_path, "%s\\maps\\%s\\%s", ".\\data\\temp", map_name, file_list.entries[file_idx].path);
                tig_net_xfer_send_as(src_path, dst_path, a1, NULL);
            }
        }

        tig_file_list_destroy(&file_list);
        multiplayer_send_player_files(a1);

        timeevent.type = TIMEEVENT_TYPE_MULTIPLAYER;
        timeevent.params[0].integer_value = 2;
        timeevent.params[1].integer_value = a1;
        timeevent.params[2].integer_value = a2;
        DateTimeAddMilliseconds(&datetime, 50);
        timeevent_add_delay(&timeevent, &datetime);
    }
}

// 0x4A3660
void multiplayer_send_player_files(int player)
{
    char pattern[TIG_MAX_PATH];
    char path[TIG_MAX_PATH];
    TigFileList file_list;
    unsigned int index;

    sprintf(pattern, "%s\\Players\\*.*", ".\\data\\temp");
    tig_file_list_create(&file_list, pattern);

    for (index = 0; index < file_list.count; index++) {
        if (strcmp(file_list.entries[index].path, ".") != 0
            && strcmp(file_list.entries[index].path, "..") != 0) {
            sprintf(path, "%s\\Players\\%s", ".\\data\\temp", file_list.entries[index].path);
            tig_net_xfer_send(path, player, NULL);
        }
    }

    tig_file_list_destroy(&file_list);
}

// 0x4A3780
void multiplayer_send_local_player_files_to_host()
{
    char prefix[40];
    char name[TIG_MAX_PATH];
    char src[TIG_MAX_PATH];
    char dst[TIG_MAX_PATH];
    int player;
    const char* exts[] = {
        ".mpc",
        ".bmp",
        "_b.bmp",
    };
    int index;

    player = sub_529520();
    objid_id_to_str(prefix, g_multiplayer_player_slots[player].field_8);

    for (index = 0; index < sizeof(exts) / sizeof(exts[0]); index++) {
        sprintf(name, "%s%s", prefix, exts[index]);
        sprintf(src, "Players\\%s", name);
        sprintf(dst, "%s\\Players", ".\\data\\temp");
        if (!tig_file_is_directory(dst)) {
            tig_file_mkdir(dst);
        }
        sprintf(dst, "%s\\Players\\%s", ".\\data\\temp", name);
        tig_file_copy(src, dst);
        tig_net_xfer_send_as(dst, dst, 0, NULL);
    }
}

// 0x4A3890
void multiplayer_nop()
{
}

// 0x4A38A0
int multiplayer_get_join_request_count()
{
    return g_multiplayer_join_request_count;
}

// 0x4A38B0
bool multiplayer_disconnect(bool (*func)(tig_button_handle_t), tig_button_handle_t button_handle)
{
    if (tig_net_is_host()) {
        char oidstr[40];
        char path[TIG_MAX_PATH];

        objid_id_to_str(oidstr, obj_get_id(player_get_local_pc_obj()));
        snprintf(path, sizeof(path), "Players\\%s.mpc", oidstr);
        if (sub_460BB0()) {
            save_char(path, player_get_local_pc_obj());
        }

        if (func != NULL) {
            func(button_handle);
        }

        tig_net_reset_connection();

        return true;
    } else {
        Packet46 pkt;
        TimeEvent timeevent;
        DateTime datetime;

        pkt.type = sub_529520();
        pkt.player = sub_529520();
        multiplayer_set_disconnect_callback(func, button_handle);
        tig_net_send_app_all(&pkt, sizeof(pkt));

        timeevent.type = TIMEEVENT_TYPE_MULTIPLAYER;
        timeevent.params[0].integer_value = 3;
        DateTimeAddMilliseconds(&datetime, 5000);
        timeevent_add_delay(&timeevent, &datetime);

        return true;
    }
}

// 0x4A39D0
void multiplayer_set_disconnect_callback(MultiplayerDisconnectCallback* func, tig_button_handle_t button_handle)
{
    g_multiplayer_disconnect_callback = func;
    g_multiplayer_disconnect_button_handle = button_handle;
}

// 0x4A39F0
bool save_char(const char* path, int64_t obj)
{
    int scheme;
    const char* scheme_rule;
    const char* scheme_name;
    char oidstr[40];
    char src_path[TIG_MAX_PATH];
    char dst_path[TIG_MAX_PATH];
    unsigned int flags;
    TigFile* stream;
    uint8_t* data;
    int size;

    multiplayer_lock();
    obj_get_id(obj);
    sub_463730(obj, true);

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_TEXT) != 0) {
        tb_remove(obj);
    }

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_TEXT_FLOATER) != 0) {
        tf_remove(obj);
    }

    magictech_handle_object_pre_remove(obj);
    anim_set_priority_level(obj, PRIORITY_HIGHEST, false, true);

    scheme = auto_level_scheme_get(obj);
    if (scheme != 0) {
        scheme_rule = auto_level_scheme_rule(scheme);
        scheme_name = auto_level_scheme_name(scheme);
    } else {
        scheme_rule = NULL;
        scheme_name = NULL;
    }

    if (scheme_rule == NULL) {
        scheme_rule = "";
    }

    if (scheme_name == NULL) {
        scheme_name = "";
    }

    objid_id_to_str(oidstr, obj_get_id(obj));

    portrait_path(portrait_get(obj), src_path, 32);
    if (src_path[0] != '\0') {
        snprintf(dst_path, sizeof(dst_path), "Players\\%s.bmp", oidstr);
        tig_file_copy(src_path, dst_path);
    }

    portrait_path(portrait_get(obj), src_path, 128);
    if (src_path[0] != '\0') {
        snprintf(dst_path, sizeof(dst_path), "Players\\%s_b.bmp", oidstr);
        tig_file_copy(src_path, dst_path);
    }

    flags = obj_field_int32_get(obj, OBJ_F_PC_FLAGS);
    flags |= OPCF_USE_ALT_DATA;
    obj_field_int32_set(obj, OPCF_USE_ALT_DATA, flags);

    multiplayer_reset_critter_state(obj);

    object_serialize_to_memory(&data, &size, obj);

    stream = tig_file_fopen(path, "wb");
    if (stream == NULL) {
        FREE(data); // FIX: Leak.
        multiplayer_unlock();
        return false;
    }

    do {
        if (tig_file_fwrite(&size, sizeof(size), 1, stream) != 1) {
            break;
        }

        if (tig_file_fwrite(&data, size, 1, stream) != 1) {
            break;
        }

        size = (int)strlen(scheme_rule) + 1;
        if (tig_file_fwrite(&size, sizeof(size), 1, stream) != 1) {
            break;
        }

        if (tig_file_fwrite(scheme_rule, sizeof(*scheme_rule), 1, stream) != 1) {
            break;
        }

        size = (int)strlen(scheme_name) + 1;
        if (tig_file_fwrite(&size, sizeof(size), 1, stream) != 1) {
            break;
        }

        if (tig_file_fwrite(scheme_name, sizeof(*scheme_rule), 1, stream) != 1) {
            break;
        }

        multiplayer_level_scheme_set(obj, scheme_rule, scheme_name);
        tig_file_fclose(stream);
        FREE(data);
        multiplayer_unlock();

        return true;
    } while (0);

    tig_file_fclose(stream);

    FREE(data);
    multiplayer_unlock();

    return false;
}

// 0x4A3D00
bool multiplayer_clear_pregen_char_list(bool a1)
{
    int idx;

    if (a1) {
        if (g_multiplayer_pregen_char_handles != NULL) {
            for (idx = 0; idx < g_multiplayer_pregen_char_count; idx++) {
                if (g_multiplayer_pregen_char_handles[idx] != OBJ_HANDLE_NULL) {
                    object_destroy(g_multiplayer_pregen_char_handles[idx]);
                }
            }
        }
    }

    if (g_multiplayer_pregen_char_handles != NULL) {
        FREE(g_multiplayer_pregen_char_handles);
    }

    g_multiplayer_pregen_char_handles = NULL;
    g_multiplayer_pregen_char_count = 0;

    return true;
}

// 0x4A3D70
bool multiplayer_load_pregen_chars(int64_t** objs_ptr, int* cnt_ptr)
{
    char path[TIG_MAX_PATH];
    TigFileList file_list;
    unsigned int idx;
    int cnt = 0;
    char* name;

    snprintf(path, sizeof(path), "Players\\%s.mpc", "G_*");
    tig_file_list_create(&file_list, path);

    if (g_multiplayer_pregen_char_handles != NULL) {
        multiplayer_clear_pregen_char_list(true);
    }

    if (file_list.count != 0) {
        g_multiplayer_pregen_char_handles = (int64_t*)CALLOC(sizeof(*g_multiplayer_pregen_char_handles), file_list.count);
        *objs_ptr = (int64_t*)CALLOC(sizeof(int64_t), file_list.count);

        for (idx = 0; idx < file_list.count; idx++) {
            snprintf(path, sizeof(path), "Players\\%s", file_list.entries[idx].path);
            if (load_char(path, &((*objs_ptr)[cnt]))) {
                obj_field_string_get((*objs_ptr)[cnt], OBJ_F_PC_PLAYER_NAME, &name);
                FREE(name);
                g_multiplayer_pregen_char_handles[cnt] = (*objs_ptr)[cnt];
                cnt++;
            } else {
                (*objs_ptr)[cnt] = OBJ_HANDLE_NULL;
                g_multiplayer_pregen_char_handles[cnt] = OBJ_HANDLE_NULL;
            }
        }

        *cnt_ptr = cnt;
        g_multiplayer_pregen_char_count = cnt;
    } else {
        g_multiplayer_pregen_char_handles = (int64_t*)CALLOC(sizeof(*g_multiplayer_pregen_char_handles), 1);
        *g_multiplayer_pregen_char_handles = OBJ_HANDLE_NULL;
        g_multiplayer_pregen_char_count = 0;

        *objs_ptr = (int64_t*)CALLOC(8, 1);
        *(*objs_ptr) = OBJ_HANDLE_NULL;
        *cnt_ptr = 0;
    }

    tig_file_list_destroy(&file_list);

    return true;
}

// 0x4A3F40
bool load_char(const char* path, int64_t* obj_ptr)
{
    TigFile* stream;
    int size;
    uint8_t* data;

    stream = tig_file_fopen(path, "rb");
    if (stream == NULL) {
        *obj_ptr = OBJ_HANDLE_NULL;
        return false;
    }

    if (tig_file_fread(&size, sizeof(size), 1, stream) != 1
        || size == 0) {
        tig_file_fclose(stream);
        *obj_ptr = OBJ_HANDLE_NULL;
        return false;
    }

    data = MALLOC(size);
    if (data == NULL) {
        tig_file_fclose(stream);
        *obj_ptr = OBJ_HANDLE_NULL;
        return false;
    }

    if (tig_file_fread(data, 1, size, stream) != size) {
        FREE(data);
        tig_file_fclose(stream);
        *obj_ptr = OBJ_HANDLE_NULL;
        return false;
    }

    tig_file_fclose(stream);

    if (!object_deserialize_from_memory(data, obj_ptr, location_make(1, 1))) {
        FREE(data);
        *obj_ptr = OBJ_HANDLE_NULL;
        return false;
    }

    if (obj_field_int32_get(*obj_ptr, OBJ_F_CRITTER_INVENTORY_NUM) != 0) {
        tig_debug_printf("MP: load_char failure, object has an inventory!\n");
    }

    if (obj_arrayfield_length_get(*obj_ptr, OBJ_F_CRITTER_INVENTORY_LIST_IDX) != 0) {
        tig_debug_printf("MP: load_char failure, object has items in inventory!\n");
    }

    if (obj_arrayfield_length_get(*obj_ptr, OBJ_F_CRITTER_FOLLOWER_IDX) != 0) {
        tig_debug_printf("MP: load_char failure, object has followers!\n");
    }

    FREE(data);

    return true;
}

// 0x4A40D0
bool multiplayer_has_char_data(int player)
{
    return g_multiplayer_char_data_slots[player] != NULL;
}

// 0x4A40F0
void multiplayer_set_char_data(int player, ObjectID oid, int level, void* data, int size)
{
    if (multiplayer_has_char_data(player)) {
        FREE(g_multiplayer_char_data_slots[player]);
    }

    // NOTE: What are 8 extra bytes for?
    g_multiplayer_char_data_slots[player] = MALLOC(sizeof(MultiplayerCharDataHeader) + 8 + size);
    g_multiplayer_char_data_slots[player]->oid = oid;
    g_multiplayer_char_data_slots[player]->level = level;
    g_multiplayer_char_data_slots[player]->size = size;
    memcpy(g_multiplayer_char_data_slots[player] + 1, data, size);
}

// 0x4A4180
void* multiplayer_get_char_data_ptr(int player)
{
    if (multiplayer_has_char_data(player)) {
        return (void*)(g_multiplayer_char_data_slots[player] + 1);
    } else {
        return NULL;
    }
}

// 0x4A41B0
int multiplayer_get_char_data_size(int player)
{
    if (multiplayer_has_char_data(player)) {
        return g_multiplayer_char_data_slots[player]->size;
    } else {
        return 0;
    }
}

// 0x4A41E0
ObjectID multiplayer_get_char_data_oid(int player)
{
    ObjectID oid;

    if (multiplayer_has_char_data(player)) {
        oid = g_multiplayer_char_data_slots[player]->oid;
    } else {
        oid.type = OID_TYPE_NULL;
    }

    return oid;
}

// 0x4A4230
void* multiplayer_get_char_data_header(int player)
{
    return g_multiplayer_char_data_slots[player];
}

// 0x4A4240
int multiplayer_get_char_data_total_size(int player)
{
    if (multiplayer_has_char_data(player)) {
        // See 0x4A40F0.
        return g_multiplayer_char_data_slots[player]->size + sizeof(MultiplayerCharDataHeader) + 8;
    } else {
        return 0;
    }
}

// 0x4A4270
void multiplayer_inc_lock_reset()
{
    g_multiplayer_reset_lock_count++;
}

// 0x4A4280
void multiplayer_dec_lock_reset()
{
    g_multiplayer_reset_lock_count--;
}

// 0x4A4320
bool multiplayer_save_local_char()
{
    char str[40];
    char path[TIG_MAX_PATH];

    objid_id_to_str(str, obj_get_id(player_get_local_pc_obj()));
    snprintf(path, sizeof(path), "Players\\%s.mpc", str);
    anim_set_priority_level(player_get_local_pc_obj(), 6, false, true);
    return save_char(path, player_get_local_pc_obj());
}

// 0x4A43B0
void multiplayer_level_scheme_set(int64_t obj, const char* rule, const char* name)
{
    int index;

    index = multiplayer_level_scheme_get(obj, NULL, NULL);
    if (index == -1) {
        index = g_multiplayer_level_scheme_next_idx;
        g_multiplayer_level_scheme_next_idx = (g_multiplayer_level_scheme_next_idx + 1) % NUM_PLAYERS;
        multiplayer_level_scheme_tbl[index].oid = obj_get_id(obj);
        multiplayer_level_scheme_tbl[index].rule[0] = '\0';
        multiplayer_level_scheme_tbl[index].name[0] = '\0';
    }

    if (rule != NULL) {
        if (rule[0] != '\0') {
            strncpy(multiplayer_level_scheme_tbl[index].rule, rule, 2000);
        } else {
            multiplayer_level_scheme_tbl[index].rule[0] = '\0';
        }
    }

    if (name != NULL) {
        if (name[0] != '\0') {
            strncpy(multiplayer_level_scheme_tbl[index].name, name, 2000);
        } else {
            multiplayer_level_scheme_tbl[index].name[0] = '\0';
        }
    }
}

// 0x4A44C0
int multiplayer_level_scheme_get(int64_t obj, char* rule, char* name)
{
    ObjectID oid;
    int index;

    oid = obj_get_id(obj);
    for (index = 0; index < NUM_PLAYERS; index++) {
        if (objid_is_equal(oid, multiplayer_level_scheme_tbl[index].oid)) {
            if (rule != NULL) {
                strcpy(rule, multiplayer_level_scheme_tbl[index].rule);
            }
            if (name != NULL) {
                strcpy(name, multiplayer_level_scheme_tbl[index].name);
            }
            return index;
        }
    }

    return -1;
}

// 0x4A45B0
bool multiplayer_notify_level_scheme_changed(int64_t obj)
{
    int size = 0;
    void* data = NULL;
    int scheme;
    const char* rule;
    const char* name;
    char str[40];
    char path[TIG_MAX_PATH];
    TigFile* stream;

    scheme = auto_level_scheme_get(obj);
    if (scheme == 0) {
        rule = auto_level_scheme_rule(scheme);
        name = auto_level_scheme_name(scheme);
    } else {
        rule = "";
        name = "";
    }

    if (rule == NULL) {
        rule = "";
    }

    if (name != NULL) {
        name = "";
    }

    multiplayer_level_scheme_set(obj, rule, name);
    objid_id_to_str(str, obj_get_id(obj));
    snprintf(path, sizeof(path), "Players\\%s.mpc", str);

    if (tig_file_exists(path, NULL)) {
        stream = tig_file_fopen(path, "rb");
        if (stream != NULL) {
            if (tig_file_fread(&size, sizeof(size), 1, stream) == 1) {
                data = MALLOC(size);
                tig_file_fread(data, size, 1, stream);
            }
            tig_file_fclose(stream);
        }

        stream = tig_file_fopen(path, "wb");
        if (stream != NULL) {
            if (tig_file_fwrite(&size, sizeof(size), 1, stream) == 1
                && tig_file_fwrite(data, size, 1, stream) == 1) {
                size = (int)strlen(rule) + 1;
                if (tig_file_fwrite(&size, sizeof(size), 1, stream) == 1) {
                    tig_file_fwrite(rule, sizeof(*rule), size, stream);

                    size = (int)strlen(name) + 1;
                    if (tig_file_fwrite(&size, sizeof(size), 1, stream) == 1) {
                        tig_file_fwrite(name, sizeof(*name), 1, stream);
                    }
                }
            }
            tig_file_fclose(stream);
        }

        FREE(data);
        sub_4EFC30(obj, name, rule);
    }

    return true;
}

// 0x4A47D0
bool multiplayer_level_scheme_rule(int64_t obj, char* str)
{
    TigFile* stream;
    char oidstr[40];
    char path[TIG_MAX_PATH];
    int pos;
    int size;

    if (multiplayer_level_scheme_get(obj, str, NULL) != -1) {
        return true;
    }

    objid_id_to_str(oidstr, obj_get_id(obj));
    snprintf(path, sizeof(path), "Players\\%s.mpc", oidstr);
    if (tig_file_exists(path, NULL)) {
        stream = tig_file_fopen(path, "rb");
        if (stream != NULL) {
            do {
                if (tig_file_fread(&pos, sizeof(pos), 1, stream) != 1) {
                    break;
                }

                if (tig_file_fseek(stream, pos, SEEK_CUR) != 0) {
                    break;
                }

                if (tig_file_fread(&size, sizeof(size), 1, stream) != 1) {
                    break;
                }

                if (tig_file_fread(str, 1, size, stream) != size) {
                    break;
                }

                multiplayer_level_scheme_set(obj, str, NULL);
                tig_file_fclose(stream);
                return true;
            } while (0);

            tig_file_fclose(stream);
        }
    }

    snprintf(path, sizeof(path), ".\\data\\temp\\Players\\%s.mpc", oidstr);
    if (tig_file_exists(path, NULL)) {
        stream = tig_file_fopen(path, "rb");
        if (stream != NULL) {
            do {
                if (tig_file_fread(&pos, sizeof(pos), 1, stream) != 1) {
                    break;
                }

                if (tig_file_fseek(stream, pos, SEEK_CUR) != 0) {
                    break;
                }

                if (tig_file_fread(&size, sizeof(size), 1, stream) != 1) {
                    break;
                }

                if (tig_file_fread(str, 1, size, stream) != size) {
                    break;
                }

                multiplayer_level_scheme_set(obj, str, NULL);
                tig_file_fclose(stream);
                return true;
            } while (0);

            tig_file_fclose(stream);
        }
    }

    *str = '\0';
    return false;
}

// 0x4A49E0
bool multiplayer_level_scheme_name(int64_t obj, char* str)
{
    TigFile* stream;
    char oidstr[40];
    char path[TIG_MAX_PATH];
    int pos;
    int size;

    if (multiplayer_level_scheme_get(obj, NULL, str) != -1) {
        return true;
    }

    objid_id_to_str(oidstr, obj_get_id(obj));
    snprintf(path, sizeof(path), "Players\\%s.mpc", oidstr);
    if (tig_file_exists(path, NULL)) {
        stream = tig_file_fopen(path, "rb");
        if (stream != NULL) {
            do {
                if (tig_file_fread(&pos, sizeof(pos), 1, stream) != 1) {
                    break;
                }

                if (tig_file_fseek(stream, pos, SEEK_CUR) != 0) {
                    break;
                }

                if (tig_file_fread(&size, sizeof(size), 1, stream) != 1) {
                    break;
                }

                if (tig_file_fseek(stream, pos, SEEK_CUR) != 0) {
                    break;
                }

                if (tig_file_fread(&size, sizeof(size), 1, stream) != 1) {
                    break;
                }

                if (tig_file_fread(str, 1, size, stream) != size) {
                    break;
                }

                multiplayer_level_scheme_set(obj, NULL, str);
                tig_file_fclose(stream);
                return true;
            } while (0);

            tig_file_fclose(stream);
        }
    }

    snprintf(path, sizeof(path), ".\\data\\temp\\Players\\%s.mpc", oidstr);
    if (tig_file_exists(path, NULL)) {
        stream = tig_file_fopen(path, "rb");
        if (stream != NULL) {
            do {
                if (tig_file_fread(&pos, sizeof(pos), 1, stream) != 1) {
                    break;
                }

                if (tig_file_fseek(stream, pos, SEEK_CUR) != 0) {
                    break;
                }

                if (tig_file_fread(&size, sizeof(size), 1, stream) != 1) {
                    break;
                }

                if (tig_file_fseek(stream, pos, SEEK_CUR) != 0) {
                    break;
                }

                if (tig_file_fread(&size, sizeof(size), 1, stream) != 1) {
                    break;
                }

                if (tig_file_fread(str, 1, size, stream) != size) {
                    break;
                }

                multiplayer_level_scheme_set(obj, NULL, str);
                tig_file_fclose(stream);
                return true;
            } while (0);

            tig_file_fclose(stream);
        }
    }

    *str = '\0';
    return false;
}

// 0x4A4C40
bool multiplayer_portrait_path(int64_t obj, int size, char* path)
{
    char str[40];

    objid_id_to_str(str, obj_get_id(obj));

    snprintf(path,
        TIG_MAX_PATH,
        "Players\\%s%s.bmp",
        str,
        size >= 128 ? "_b" : "");
    if (tig_file_exists(path, NULL)) {
        return true;
    }

    snprintf(path,
        TIG_MAX_PATH,
        "%s\\Players\\%s%s.bmp",
        ".\\data\\temp",
        str,
        size >= 128 ? "_b" : "");
    if (tig_file_exists(path, NULL)) {
        return true;
    }

    snprintf(path,
        TIG_MAX_PATH,
        "%s\\Players\\%s%s.bmp",
        "Save\\Current",
        str,
        size >= 128 ? "_b" : "");
    if (tig_file_exists(path, NULL)) {
        return true;
    }

    path[0] = '\0';
    return false;
}

// 0x4A50D0
bool multiplayer_hide_item(int64_t pc_obj, int64_t item_obj)
{
    unsigned int flags;

    if (tig_net_is_active()) {
        int client_id;
        Packet93 pkt;
        int64_t parent_obj;

        if (pc_obj == player_get_local_pc_obj()) {
            client_id = multiplayer_find_slot_from_obj(pc_obj);
            if (client_id != -1) {
                if (multiplayer_add_hidden_item(client_id, item_obj)) {
                    pkt.type = 93;
                    pkt.field_4 = client_id;
                    pkt.field_20 = 0;
                    pkt.oid = obj_get_id(item_obj);
                    tig_net_send_app_all(&pkt, sizeof(pkt));
                }
            }
        }

        ui_update_inven(item_obj);

        if (item_parent(item_obj, &parent_obj)) {
            ui_update_inven(parent_obj);
        }

        return true;
    }

    flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
    flags |= OIF_NO_DISPLAY;
    obj_field_int32_set(item_obj, OBJ_F_ITEM_FLAGS, flags);

    return true;
}

// 0x4A51C0
bool multiplayer_show_item(int64_t pc_obj, int64_t item_obj)
{
    unsigned int flags;

    if (tig_net_is_active()) {
        int client_id;
        Packet93 pkt;

        if (pc_obj != player_get_local_pc_obj()) {
            return false;
        }

        client_id = multiplayer_find_slot_from_obj(pc_obj);
        if (client_id == -1) {
            return false;
        }

        if (!multiplayer_remove_hidden_item(client_id)) {
            return false;
        }

        pkt.type = 93;
        pkt.field_4 = client_id;
        pkt.field_20 = 1;
        pkt.oid = obj_get_id(item_obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
        return true;
    }

    flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
    flags &= ~OIF_NO_DISPLAY;
    obj_field_int32_set(item_obj, OBJ_F_ITEM_FLAGS, flags);

    return true;
}

// 0x4A5290
void multiplayer_init_hidden_items()
{
    g_multiplayer_hidden_items[0] = OBJ_HANDLE_NULL;
    memmove(&(g_multiplayer_hidden_items[1]),
        &(g_multiplayer_hidden_items[0]),
        sizeof(g_multiplayer_hidden_items) - sizeof(g_multiplayer_hidden_items[0]));
}

// 0x4A52C0
bool multiplayer_add_hidden_item(int client_id, int64_t item_obj)
{
    unsigned int flags;

    if (g_multiplayer_hidden_items[client_id] != OBJ_HANDLE_NULL) {
        return false;
    }

    flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
    flags |= OIF_NO_DISPLAY;
    obj_field_int32_set(item_obj, OBJ_F_ITEM_FLAGS, flags);

    g_multiplayer_hidden_items[client_id] = item_obj;

    return true;
}

// 0x4A5320
bool multiplayer_remove_hidden_item(int client_id)
{
    unsigned int flags;

    if (g_multiplayer_hidden_items[client_id] == OBJ_HANDLE_NULL) {
        return false;
    }

    flags = obj_field_int32_get(g_multiplayer_hidden_items[client_id], OBJ_F_ITEM_FLAGS);
    flags &= ~OIF_NO_DISPLAY;
    obj_field_int32_set(g_multiplayer_hidden_items[client_id], OBJ_F_ITEM_FLAGS, flags);

    g_multiplayer_hidden_items[client_id] = OBJ_HANDLE_NULL;

    return true;
}

// 0x4A5380
void multiplayer_init_trade_list()
{
    g_multiplayer_trade_partners[0] = OBJ_HANDLE_NULL;
    memmove(&(g_multiplayer_trade_partners[1]),
        &(g_multiplayer_trade_partners[0]),
        sizeof(g_multiplayer_trade_partners) - sizeof(g_multiplayer_trade_partners[0]));
}

// 0x4A53B0
void multiplayer_set_active_trade_partner(int64_t a1, int64_t a2)
{
    int client_id;

    if (!multiplayer_is_locked()) {
        Packet97 pkt;

        pkt.type = 97;
        pkt.field_8 = obj_get_id(a1);
        if (a2 != OBJ_HANDLE_NULL) {
            pkt.field_20 = obj_get_id(a2);
        } else {
            pkt.field_20.type = OID_TYPE_NULL;
        }
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    client_id = multiplayer_find_slot_from_obj(a1);
    if (client_id != -1) {
        g_multiplayer_trade_partners[client_id] = a2;
    }
}

// 0x4A5460
int multiplayer_get_trade_partner_count(int64_t a1)
{
    int cnt = 0;
    int index;

    for (index = 0; index < NUM_PLAYERS; index++) {
        if (g_multiplayer_trade_partners[index] == a1) {
            cnt++;
        }
    }

    return cnt;
}

// 0x4A5490
void multiplayer_ping(tig_timestamp_t timestamp)
{
    (void)timestamp;
}

// 0x4A54A0
void multiplayer_anim_ping_handler()
{
    int index;
    char str[ANIM_ID_STR_SIZE];

    for (index = 0; index < 216; index++) {
        if ((anim_run_info[index].flags & 0x1) != 0) {
            if (!anim_goal_restart(&(anim_run_info[index].id))) {
                // FIXME: Meaningless.
                anim_id_to_str(&(anim_run_info[index].id), str);
            }
        }
    }
}

// 0x4A54E0
void multiplayer_magictech_ping_handler()
{
    int index;

    for (index = 0; index < 512; index++) {
        if ((magictech_run_info[index].flags & MAGICTECH_RUN_ACTIVE) != 0) {
            magictech_reschedule_timeevent_after_load(index);
        }
    }
}

// 0x4A5510
void multiplayer_flags_set(int client_id, unsigned int flags)
{
    PacketMultiplayerFlagsChange pkt;

    g_multiplayer_player_slots[client_id].flags |= flags & 0xFF00;

    pkt.type = 98;
    pkt.client_id = client_id;
    pkt.flags = g_multiplayer_player_slots[client_id].flags & 0xFF00;
    tig_net_send_app_all(&pkt, sizeof(pkt));
}

// 0x4A5570
void multiplayer_flags_unset(int client_id, unsigned int flags)
{
    PacketMultiplayerFlagsChange pkt;

    g_multiplayer_player_slots[client_id].flags &= ~(flags & 0xFF00);

    pkt.type = 98;
    pkt.client_id = client_id;
    pkt.flags = g_multiplayer_player_slots[client_id].flags & 0xFF00;
    tig_net_send_app_all(&pkt, sizeof(pkt));
}

// 0x4A55D0
unsigned int multiplayer_flags_get(int client_id)
{
    return g_multiplayer_player_slots[client_id].flags & 0xFF00;
}

// 0x4A55F0
int multiplayer_get_difficulty()
{
    return g_multiplayer_current_difficulty;
}

// 0x4A5600
void multiplayer_set_difficulty(int a1)
{
    g_multiplayer_current_difficulty = a1;
}

// 0x4A5610
void multiplayer_apply_settings()
{
    set_always_run(settings_get_value(&settings, ALWAYS_RUN_KEY));
    combat_auto_attack_set(settings_get_value(&settings, AUTO_ATTACK_KEY));
    combat_auto_switch_weapons_set(settings_get_value(&settings, AUTO_SWITCH_WEAPON_KEY));
    set_follower_skills(settings_get_value(&settings, FOLLOWER_SKILLS_KEY));
}

// 0x4A5670
void multiplayer_auto_equip_char(int64_t obj)
{
    mes_file_handle_t mes_file;

    if (mes_load("Rules\\AutoEquip.mes", &mes_file)) {
        if ((tig_net_local_server_get_options() & TIG_NET_SERVER_AUTO_EQUIP) == 0) {
            tig_str_parse_set_separator(' ');
            multiplayer_auto_equip_give_money(obj, mes_file);
            multiplayer_auto_equip_give_background_items(obj);
            multiplayer_auto_equip_give_schematic_items(obj);
            multiplayer_auto_equip_give_armor(obj, mes_file);
            multiplayer_auto_equip_give_weapon_items(obj, mes_file);
            multiplayer_auto_equip_give_skill_items(obj, mes_file);
            item_wield_best_all(obj, OBJ_HANDLE_NULL);
        }
    }
}

// 0x4A5710
int multiplayer_auto_equip_give_money(int64_t obj, mes_file_handle_t mes_file)
{
    int64_t location;
    int level;
    MesFileEntry mes_file_entry;
    int money_amt;
    int64_t money_obj;

    location = obj_field_int64_get(obj, OBJ_F_LOCATION);
    level = stat_level_get(obj, STAT_LEVEL);

    mes_file_entry.num = (level + 9) / 10;
    if (!mes_search(mes_file, &mes_file_entry)) {
        return 0;
    }

    mes_get_msg(mes_file, &mes_file_entry);
    money_amt = atoi(mes_file_entry.str);
    money_amt = background_adjust_money(money_amt, background_get(obj));
    if (money_amt > 0) {
        mp_object_create(BP_GOLD, location, &money_obj);
        mp_obj_field_int32_set(money_obj, OBJ_F_GOLD_QUANTITY, money_amt);
        item_transfer(money_obj, obj);
    }
    return money_amt;
}

// 0x4A57F0
int multiplayer_auto_equip_give_background_items(int64_t obj)
{
    char buffer[2000];

    background_get_items(buffer, sizeof(buffer), background_get(obj));

    return multiplayer_auto_equip_give_items_from_string(obj, buffer);
}

// 0x4A5840
int multiplayer_auto_equip_give_skill_items(int64_t obj, mes_file_handle_t mes_file)
{
    int v1 = 0;
    int level;
    int value;

    level = stat_level_get(obj, STAT_LEVEL);

    value = tech_skill_level(obj, TECH_SKILL_PICK_LOCKS);
    if (value > 0) {
        v1 += multiplayer_auto_equip_give_items_from_mes(obj, mes_file, (value + level + 29) / 30 + 1100);
    }

    value = basic_skill_level(obj, BASIC_SKILL_HEAL);
    if (value > 0) {
        v1 += multiplayer_auto_equip_give_items_from_mes(obj, mes_file, (value + level + 29) / 30 + 1200);
    }

    value = basic_skill_level(obj, BASIC_SKILL_BACKSTAB);
    if (value > 0) {
        v1 += multiplayer_auto_equip_give_items_from_mes(obj, mes_file, (value + level + 29) / 30 + 1300);
    }

    return v1;
}

// 0x4A5920
int multiplayer_auto_equip_give_items_from_mes(int64_t obj, mes_file_handle_t mes_file, int num)
{
    MesFileEntry mes_file_entry1;
    MesFileEntry mes_file_entry2;

    mes_file_entry1.num = num;
    mes_file_entry2.num = num + 10;

    if (mes_search(mes_file, &mes_file_entry2)) {
        mes_get_msg(mes_file, &mes_file_entry2);
        if (multiplayer_auto_equip_check_has_items(obj, mes_file_entry2.str)) {
            return 0;
        }
    }

    if (mes_search(mes_file, &mes_file_entry1)) {
        mes_get_msg(mes_file, &mes_file_entry1);
        return multiplayer_auto_equip_give_items_from_string(obj, mes_file_entry1.str);
    }

    return 0;
}

// 0x4A59F0
int multiplayer_auto_equip_give_weapon_items(int64_t obj, mes_file_handle_t mes_file)
{
    int v1 = 0;
    int level;
    int value;
    MesFileEntry mes_file_entry;

    level = stat_level_get(obj, STAT_LEVEL);

    // Melee Weapons
    value = basic_skill_level(obj, BASIC_SKILL_MELEE);
    if (value > 0) {
        mes_file_entry.num = 600;
        if (mes_search(mes_file, &mes_file_entry)) {
            mes_get_msg(mes_file, &mes_file_entry);
            v1 += multiplayer_auto_equip_give_items_from_string(obj, mes_file_entry.str);
        }
        v1 += multiplayer_auto_equip_give_items_from_mes(obj, mes_file, (value + level + 9) / 10 + 600);
    }

    // Bows
    value = basic_skill_level(obj, BASIC_SKILL_BOW);
    if (value > 0) {
        mes_file_entry.num = 700;
        if (mes_search(mes_file, &mes_file_entry)) {
            mes_get_msg(mes_file, &mes_file_entry);
            v1 += multiplayer_auto_equip_give_items_from_string(obj, mes_file_entry.str);
        }
        v1 += multiplayer_auto_equip_give_items_from_mes(obj, mes_file, (value + level + 9) / 10 + 700);
    }

    // Throwing
    value = basic_skill_level(obj, BASIC_SKILL_THROWING);
    if (value > 0) {
        mes_file_entry.num = 800;
        if (mes_search(mes_file, &mes_file_entry)) {
            mes_get_msg(mes_file, &mes_file_entry);
            v1 += multiplayer_auto_equip_give_items_from_string(obj, mes_file_entry.str);
        }
        v1 += multiplayer_auto_equip_give_items_from_mes(obj, mes_file, (value + level + 9) / 10 + 800);
    }

    // Firearms
    value = tech_skill_level(obj, TECH_SKILL_FIREARMS);
    if (value > 0) {
        mes_file_entry.num = 900;
        if (mes_search(mes_file, &mes_file_entry)) {
            mes_get_msg(mes_file, &mes_file_entry);
            v1 += multiplayer_auto_equip_give_items_from_string(obj, mes_file_entry.str);
        }
        v1 += multiplayer_auto_equip_give_items_from_mes(obj, mes_file, (value + level + 9) / 10 + 900);
    }

    if (v1 != 0) {
        return v1;
    }

    // Fail safe weapons (if you didn't get any of the above).
    if (level <= 20) {
        // {6029} Dagger
        mes_file_entry.num = 1001;
    } else {
        if (stat_level_get(obj, STAT_MAGICK_POINTS) >= 75) {
            // {6163} Mage's Staff
            mes_file_entry.num = 1002;
        } else if (stat_level_get(obj, STAT_MAGICK_POINTS) >= 20) {
            // {6054} Magic Dagger
            mes_file_entry.num = 1003;
        } else if (stat_level_get(obj, STAT_TECH_POINTS) >= 75) {
            // {6053} Mechanical Dagger
            mes_file_entry.num = 1004;
        } else {
            // {6051} Quality Dagger
            mes_file_entry.num = 1005;
        }
    }

    mes_get_msg(mes_file, &mes_file_entry);

    return multiplayer_auto_equip_give_items_from_string(obj, mes_file_entry.str);
}

// 0x4A5CA0
void multiplayer_auto_equip_give_armor(int64_t obj, mes_file_handle_t mes_file)
{
    int64_t loc;
    int race;
    int wearable_armor_size;
    int level;
    int strength;
    int score;
    int num;

    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    race = stat_level_get(obj, STAT_RACE);
    wearable_armor_size = item_armor_size(race);

    switch (wearable_armor_size) {
    case OARF_SIZE_SMALL:
        num = stat_level_get(obj, STAT_MAGICK_POINTS) < 50 ? 400 : 200;
        break;
    case OARF_SIZE_MEDIUM:
        num = stat_level_get(obj, STAT_MAGICK_POINTS) < 50 ? 300 : 100;
        break;
    case OARF_SIZE_LARGE:
        num = 500;
        break;
    default:
        // Should be unreachable.
        assert(0);
    }

    level = stat_level_get(obj, STAT_LEVEL);
    strength = stat_level_get(obj, STAT_STRENGTH);
    score = strength + level - 21;
    if (score < 1) {
        num += 1;
    } else if (score > 50) {
        num += 5;
    } else {
        num += (strength + level - 12) / 10;
    }

    multiplayer_auto_equip_give_items_from_mes(obj, mes_file, num);
}

// 0x4A5D80
int multiplayer_auto_equip_check_has_items(int64_t obj, char* str)
{
    int cnt = 0;
    int basic_proto;

    while (str != NULL && *str != '\0') {
        while (SDL_isspace(*str)) {
            str++;
        }

        if (*str == '\0') {
            break;
        }

        tig_str_parse_value(&str, &basic_proto);
        if (sub_462540(obj, GetProtoHandleFromID(basic_proto), 0)) {
            cnt++;
        }
    }

    return cnt;
}

// 0x4A5E10
int multiplayer_auto_equip_give_items_from_string(int64_t obj, char* str)
{
    int64_t loc;
    int cnt = 0;
    int basic_proto;
    int64_t item_obj;

    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    while (str != NULL && *str != '\0') {
        while (SDL_isspace(*str)) {
            str++;
        }

        if (*str == '\0') {
            break;
        }

        tig_str_parse_value(&str, &basic_proto);
        mp_object_create(basic_proto, loc, &item_obj);
        item_transfer(item_obj, obj);
        cnt++;
    }

    return cnt;
}

// 0x4A5EE0
bool multiplayer_auto_equip_give_schematic_items(int64_t obj)
{
    int64_t loc;
    int race;
    int wearable_armor_size;
    int tech;
    int degree;
    int schematic;
    SchematicInfo schematic_info;
    int idx;
    int prod_basic_proto;
    int64_t prod_proto_obj;
    int size;
    int qty;
    int64_t item_obj;

    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    race = stat_level_get(obj, STAT_RACE);
    wearable_armor_size = item_armor_size(race);

    for (tech = 0; tech < TECH_COUNT; tech++) {
        degree = tech_degree_get(obj, tech);
        if (degree > 0) {
            schematic = tech_schematic_get(tech, degree);
            ui_schematic_info_get(schematic, &schematic_info);

            for (idx = 0; idx < 3; idx++) {
                prod_basic_proto = schematic_info.prod[idx];
                prod_proto_obj = GetProtoHandleFromID(schematic_info.prod[idx]);
                if (obj_field_int32_get(prod_proto_obj, OBJ_F_TYPE) != OBJ_TYPE_ARMOR) {
                    break;
                }
                size = obj_field_int32_get(prod_proto_obj, OBJ_F_ARMOR_FLAGS) & (OARF_SIZE_SMALL | OARF_SIZE_MEDIUM | OARF_SIZE_LARGE);
                if (size == 0 || size == wearable_armor_size) {
                    break;
                }
            }

            if (idx != 3) {
                for (qty = 0; qty < schematic_info.qty; qty++) {
                    mp_object_create(prod_basic_proto, loc, &item_obj);
                    item_transfer(item_obj, obj);
                }
            }
        }
    }

    return true;
}

// 0x4A6010
void multiplayer_reset_critter_state(int64_t obj)
{
    unsigned int flags;
    int poison;
    int index;

    object_hp_damage_set(obj, 0);
    mp_critter_fatigue_damage_set(obj, 0);
    mp_object_flags_unset(obj, OF_OFF);
    mp_object_flags_unset(obj, OF_FLAT);

    flags = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS);
    flags &= ~0xFC03D0FF;
    mp_obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS, flags);

    flags = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS2);
    flags &= ~0x3FFFFF;
    mp_obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS2, flags);

    if (combat_critter_is_combat_mode_active(obj)) {
        combat_critter_deactivate_combat_mode(obj);
    }

    poison = stat_base_get(obj, STAT_POISON_LEVEL);
    if (poison != 0) {
        stat_base_set(obj, STAT_POISON_LEVEL, poison);
    }

    for (index = 0; index < 10; index++) {
        if (index < 0 || index > 2) {
            effect_remove_one_caused_by(obj, index);
        }
    }

    sub_4F0500(obj, OBJ_F_PC_QUEST_IDX);
    sub_4F0500(obj, OBJ_F_PC_RUMOR_IDX);
    sub_4F0500(obj, OBJ_F_PC_BLESSING_IDX);
    sub_4F0500(obj, OBJ_F_PC_BLESSING_TS_IDX);
    sub_4F0500(obj, OBJ_F_PC_CURSE_IDX);
    sub_4F0500(obj, OBJ_F_PC_CURSE_TS_IDX);
    sub_4F0500(obj, OBJ_F_PC_REPUTATION_IDX);
    sub_4F0500(obj, OBJ_F_PC_REPUTATION_TS_IDX);
    sub_4F0500(obj, OBJ_F_CRITTER_FOLLOWER_IDX);
    sub_4F0500(obj, OBJ_F_PC_GLOBAL_FLAGS);
    sub_4F0500(obj, OBJ_F_PC_GLOBAL_VARIABLES);
    sub_4F0500(obj, OBJ_F_SPELL_FLAGS);

    for (index = 0; index < 7; index++) {
        mp_object_overlay_set(obj, OBJ_F_OVERLAY_FORE, index, TIG_ART_ID_INVALID);
        mp_object_overlay_set(obj, OBJ_F_OVERLAY_BACK, index, TIG_ART_ID_INVALID);
    }
}

// 0x4A6190
bool multiplayer_is_pvp_damage_disallowed(int64_t a1, int64_t a2, int64_t a3, int64_t a4)
{
    if ((tig_net_local_server_get_options() & TIG_NET_SERVER_PLAYER_KILLING) == 0
        && a1 != a2) {
        if (a1 != OBJ_HANDLE_NULL
            && obj_field_int32_get(a1, OBJ_F_TYPE) == OBJ_TYPE_PC
            && a2 != OBJ_HANDLE_NULL) {
            if (obj_field_int32_get(a2, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                return true;
            }

            if (obj_field_int32_get(a3, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                return true;
            }
        }

        if (a4 != OBJ_HANDLE_NULL
            && obj_field_int32_get(a4, OBJ_F_TYPE) == OBJ_TYPE_PC
            && a2 != OBJ_HANDLE_NULL) {
            if (obj_field_int32_get(a2, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                return true;
            }

            if (obj_field_int32_get(a3, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                return true;
            }
        }
    }

    if ((tig_net_local_server_get_options() & TIG_NET_SERVER_FRIENDLY_FIRE) == 0
        && a1 != a2) {
        if (a1 != OBJ_HANDLE_NULL
            && (obj_field_int32_get(a1, OBJ_F_TYPE) == OBJ_TYPE_PC
                || obj_field_int32_get(a1, OBJ_F_TYPE) == OBJ_TYPE_NPC)) {
            if (a2 != OBJ_HANDLE_NULL
                && (obj_field_int32_get(a2, OBJ_F_TYPE) == OBJ_TYPE_PC
                    || obj_field_int32_get(a2, OBJ_F_TYPE) == OBJ_TYPE_NPC)
                && critter_party_same(a1, a2)) {
                return true;
            }

            if (a3 != OBJ_HANDLE_NULL
                && (obj_field_int32_get(a3, OBJ_F_TYPE) == OBJ_TYPE_PC
                    || obj_field_int32_get(a3, OBJ_F_TYPE) == OBJ_TYPE_NPC)
                && critter_party_same(a1, a3)) {
                return true;
            }
        }

        if (a4 != OBJ_HANDLE_NULL
            && (obj_field_int32_get(a4, OBJ_F_TYPE) == OBJ_TYPE_PC
                || obj_field_int32_get(a4, OBJ_F_TYPE) == OBJ_TYPE_NPC)) {
            if (a2 != OBJ_HANDLE_NULL
                && (obj_field_int32_get(a2, OBJ_F_TYPE) == OBJ_TYPE_PC
                    || obj_field_int32_get(a2, OBJ_F_TYPE) == OBJ_TYPE_NPC)
                && critter_party_same(a1, a2)) {
                return true;
            }

            if (a3 != OBJ_HANDLE_NULL
                && (obj_field_int32_get(a3, OBJ_F_TYPE) == OBJ_TYPE_PC
                    || obj_field_int32_get(a3, OBJ_F_TYPE) == OBJ_TYPE_NPC)
                && critter_party_same(a1, a3)) {
                return true;
            }
        }
    }

    return false;
}

// 0x4A6470
bool multiplayer_save_char_with_prompt(int64_t pc_obj)
{
    char oidstr[40];
    char path[TIG_MAX_PATH];
    char str[256];

    objid_id_to_str(oidstr, obj_get_id(pc_obj));
    snprintf(path, sizeof(path), "Players\\%s.mpc", oidstr);
    anim_set_priority_level(player_get_local_pc_obj(), PRIORITY_HIGHEST, false, true);

    if (sub_460BC0()) {
        if (tig_file_exists(path, NULL)) {
            object_examine(pc_obj, pc_obj, str);
            if (!sub_460BE0(str, oidstr)) {
                return false;
            }
        }

        if (!save_char(path, pc_obj)) {
            return false;
        }
    }

    return true;
}

// 0x4A6560
bool multiplayer_strip_guid_from_filename(const char* a1, char* a2)
{
    static const struct {
        char ch;
        int pos;
    } meta[7] = {
        { '-', 0 },
        { '{', 1 },
        { '-', 10 },
        { '-', 15 },
        { '-', 20 },
        { '-', 25 },
        { '}', 38 },
    };

    char path[TIG_MAX_PATH];
    char dir[COMPAT_MAX_DIR];
    char fname[COMPAT_MAX_FNAME];
    char ext[COMPAT_MAX_EXT];
    int pos;
    int idx;

    strncpy(path, a1, sizeof(path));
    compat_splitpath(path, NULL, dir, fname, ext);

    pos = (int)strlen(fname);
    if (pos > 39) {
        for (idx = 0; idx < 7; idx++) {
            if (fname[pos - 39 + meta[idx].pos] != meta[idx].ch) {
                return false;
            }
        }

        if (a2 != NULL) {
            fname[pos] = '\0';
            compat_makepath(a2, NULL, dir, fname, ext);
        }

        return true;
    }

    return false;
}
