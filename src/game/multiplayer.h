#ifndef ARCANUM_GAME_MULTIPLAYER_H_
#define ARCANUM_GAME_MULTIPLAYER_H_

#include "game/context.h"
#include "game/obj.h"
#include "game/timeevent.h"

#define MULTIPLAYER_ALWAYS_RUN 0x0100u
#define MULTIPLAYER_AUTO_ATTACK 0x0200u
#define MULTIPLAYER_AUTO_SWITCH_WEAPONS 0x0400u
#define MULTIPLAYER_FOLLOWER_SKILLS 0x0800u

typedef void(MultiplayerClientJoinedCallback)();
typedef bool(MultiplayerDisconnectCallback)(tig_button_handle_t button_handle);

bool multiplayer_init(GameInitInfo* init_info);
void multiplayer_exit();
void multiplayer_reset();
bool multiplayer_save(TigFile* stream);
bool mutliplayer_load(GameLoadInfo* load_info);
bool multiplayer_mod_load();
void multiplayer_mod_unload();
bool multiplayer_start();
bool multiplayer_end();
void multiplayer_start_server();
bool multiplayer_load_module_and_start(const char* a1, const char* a2);
bool multiplayer_timeevent_process(TimeEvent* timeevent);
bool multiplayer_map_open_by_name(const char* name);
void multiplayer_set_client_joined_callback(MultiplayerClientJoinedCallback* func);
int multiplayer_find_slot_from_obj(int64_t obj);
int64_t multiplayer_get_player_obj_by_slot(int player);
bool multiplayer_is_locked();
void multiplayer_lock();
void multiplayer_unlock();
int64_t multiplayer_player_find_first();
int64_t multiplayer_player_find_next();
void multiplayer_request_object_lock(ObjectID oid, bool (*success_func)(void*), void* success_info, bool (*failure_func)(void*), void* failure_info);
void multiplayer_nop();
int multiplayer_get_join_request_count();
bool multiplayer_disconnect(bool (*func)(tig_button_handle_t), tig_button_handle_t button_handle);
void multiplayer_set_disconnect_callback(MultiplayerDisconnectCallback* func, tig_button_handle_t button_handle);
bool multiplayer_clear_pregen_char_list(bool a1);
bool multiplayer_load_pregen_chars(int64_t** objs_ptr, int* cnt_ptr);
void multiplayer_set_char_data(int player, ObjectID oid, int level, void* data, int size);
void* multiplayer_get_char_data_ptr(int player);
int multiplayer_get_char_data_size(int player);
ObjectID multiplayer_get_char_data_oid(int player);
void* multiplayer_get_char_data_header(int player);
int multiplayer_get_char_data_total_size(int player);
void multiplayer_inc_lock_reset();
void multiplayer_dec_lock_reset();
bool multiplayer_save_local_char();
bool multiplayer_notify_level_scheme_changed(int64_t obj);
bool multiplayer_level_scheme_rule(int64_t obj, char* str);
bool multiplayer_level_scheme_name(int64_t obj, char* str);
bool multiplayer_portrait_path(int64_t obj, int size, char* path);
bool multiplayer_hide_item(int64_t pc_obj, int64_t item_obj);
bool multiplayer_show_item(int64_t pc_obj, int64_t item_obj);
int multiplayer_get_trade_partner_count(int64_t a1);
void multiplayer_ping(tig_timestamp_t timestamp);
void multiplayer_set_active_trade_partner(int64_t a1, int64_t a2);
void multiplayer_flags_set(int client_id, unsigned int flags);
void multiplayer_flags_unset(int client_id, unsigned int flags);
unsigned int multiplayer_flags_get(int client_id);
int multiplayer_get_difficulty();
void multiplayer_set_difficulty(int a1);
void multiplayer_apply_settings();
bool multiplayer_is_pvp_damage_disallowed(int64_t a1, int64_t a2, int64_t a3, int64_t a4);
bool multiplayer_save_char_with_prompt(int64_t pc_obj);

#endif /* ARCANUM_GAME_MULTIPLAYER_H_ */
