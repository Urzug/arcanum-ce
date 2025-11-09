#ifndef ARCANUM_GAME_FACADE_H_
#define ARCANUM_GAME_FACADE_H_

#include "game/context.h"
#include "game/location.h"

bool facade_init(GameInitInfo* init_info);
void facade_exit();
void facade_resize(GameResizeInfo* resize_info);
void facade_update_view(ViewOptions* view_options);
void facade_draw(GameDrawInfo* draw_info);
bool facade_clip_rect_and_get_start_indices(LocRect* loc_rect, int* a2, int* a3);

#endif /* ARCANUM_GAME_FACADE_H_ */
