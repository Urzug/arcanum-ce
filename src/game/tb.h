#ifndef ARCANUM_GAME_TB_H_
#define ARCANUM_GAME_TB_H_

#include "game/context.h"

#define TB_EXPIRE_NEVER -2
#define TB_EXPIRE_DEFAULT -1

typedef enum TbType {
    TB_TYPE_WHITE,
    TB_TYPE_RED,
    TB_TYPE_GREEN,
    TB_TYPE_BLUE,
    TB_TYPE_COUNT,
} TbType;

bool tb_init(GameInitInfo* init_info);
void tb_reset();
void tb_exit();
void tb_resize(GameResizeInfo* resize_info);
bool tb_update_view(ViewOptions* view_options);
void tb_close();
void tb_toggle();
void tb_ping(tig_timestamp_t time);
void tb_render(UnknownContext* render_info);
void tb_add(int64_t obj, int type, const char* str);
void tb_expire_in(int64_t obj, int seconds);
void tb_notify_moved(int64_t obj, int64_t loc, int offset_x, int offset_y);
void tb_remove(int64_t obj);
void tb_clear();

#endif /* ARCANUM_GAME_TB_H_ */
