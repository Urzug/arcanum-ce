#ifndef ARCANUM_GAME_CI_H_
#define ARCANUM_GAME_CI_H_

#include "game/context.h"

bool ci_init(GameInitInfo* init_info);
void ci_exit();
void ui_tb_lock_cursor();
void ui_tb_lock_cursor_maybe();
int ui_is_tb_cursor_locked();
void ui_tb_unlock_cursor();
void ci_redraw();

#endif /* ARCANUM_GAME_CI_H_ */
