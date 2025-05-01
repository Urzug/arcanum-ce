#ifndef ARCANUM_GAME_MULTIPLAYER_H_
#define ARCANUM_GAME_MULTIPLAYER_H_

#include "game/context.h"
#include "game/obj.h"
#include "game/matchmaker.h"
#include "game/timeevent.h"

typedef void(Func5F0E08)();
typedef bool(Func5F0DF8)(tig_button_handle_t button_handle);

bool multiplayer_init(GameInitInfo* init_info);
void multiplayer_exit();
void multiplayer_reset();
bool multiplayer_save(TigFile* stream);
bool mutliplayer_load(GameLoadInfo* load_info);
bool multiplayer_mod_load();
void multiplayer_mod_unload();
bool multiplayer_start();
bool sub_49CC20();
void sub_49CC50();
bool sub_49CC70(const char* a1, const char* a2);
bool multiplayer_timeevent_process(TimeEvent* timeevent);
bool multiplayer_map_open_by_name(const char* name);
void sub_4A2B00(Func5F0E08* func);
int sub_4A2B10(int64_t obj);
int64_t sub_4A2B60(int player);
bool multiplayer_is_locked();
void multiplayer_lock();
void multiplayer_unlock();
int64_t multiplayer_find_first_player_obj();
int64_t multiplayer_find_next_player_obj();
void sub_4A3230(ObjectID oid, bool(*success_func)(void*), void* success_info, bool(*failure_func)(void*), void* failure_info);
void sub_4A3890();
int sub_4A38A0();
bool sub_4A38B0(bool(*func)(tig_button_handle_t), tig_button_handle_t button_handle);
void sub_4A39D0(Func5F0DF8* func, tig_button_handle_t button_handle);
bool sub_4A3D00(bool a1);
bool sub_4A3D70(int64_t** objs_ptr, int* cnt_ptr);
void sub_4A40F0(int player, ObjectID oid, int level, void* data, int size);
void* sub_4A4180(int player);
int sub_4A41B0(int player);
ObjectID sub_4A41E0(int player);
void* sub_4A4230(int player);
int sub_4A4240(int player);
void sub_4A4270();
void sub_4A4280();
bool sub_4A4320();
bool sub_4A45B0(int64_t obj);
bool sub_4A47D0(int64_t obj, char* str);
bool sub_4A49E0(int64_t obj, char* str);
bool sub_4A4C40(int64_t obj, int size, char* path);
void sub_4A4D60(MatchmakerInitInfo* init_info);
int multiplayer_mm_init(MatchmakerInitInfo* init_info);
void multiplayer_mm_exit();
int multiplayer_mm_is_active();
int multiplayer_mm_ping();
int multiplayer_mm_motd_get(char* a1, int a2, char* a3, int a4);
int multiplayer_mm_login(const char* account, const char* password);
int multiplayer_mm_create_account(const char* account, const char* password, const char* a3);
int multiplayer_mm_version_needs_upgrade(const char* version);
int multiplayer_mm_ad_rgb_get(uint8_t** rgb_ptr, int* width_ptr, int* height_ptr);
int multiplayer_mm_ad_release();
int multiplayer_mm_ad_clicked();
int multiplayer_mm_register(int a1);
int multiplayer_mm_gamelist_get(MatchmakerRegisterInfo** games, int* count);
int multiplayer_mm_gamelist_free(MatchmakerRegisterInfo* games);
int multiplayer_mm_chatserver_list_get(void** chatservers, int* count);
int multiplayer_mm_chatserver_list_free(void* chatservers);
int multiplayer_mm_chatserver_join(int a1);
int multiplayer_mm_chatroom_list_get(MatchmakerChatroom** chatrooms, int* count);
int multiplayer_mm_chatroom_list_free(MatchmakerChatroom* chatrooms);
int multiplayer_mm_chatroom_join(MatchmakerChatroom* room, int a2);
int multiplayer_mm_chatroom_members_get(MatchmakerChatroomMember** members, int* count);
int multiplayer_mm_chatroom_members_free(MatchmakerChatroomMember* members);
int multiplayer_mm_chatroom_create(const char* a1, const char* a2);
int multiplayer_mm_chatroom_mesg(const char* msg);
bool sub_4A50D0(int64_t pc_obj, int64_t item_obj);
bool sub_4A51C0(int64_t pc_obj, int64_t item_obj);
int sub_4A5460(int64_t a1);
void multiplayer_ping(tig_timestamp_t timestamp);
void sub_4A53B0(int64_t a1, int64_t a2);
void sub_4A5510(int client_id, unsigned int flags);
void sub_4A5570(int client_id, unsigned int flags);
int sub_4A55D0(int player);
int sub_4A55F0();
void sub_4A5600(int a1);
void sub_4A5610();
bool sub_4A6190(int64_t a1, int64_t a2, int64_t a3, int64_t a4);
bool sub_4A6470(int64_t pc_obj);

#endif /* ARCANUM_GAME_MULTIPLAYER_H_ */
