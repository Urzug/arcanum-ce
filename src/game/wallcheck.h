#ifndef ARCANUM_GAME_WALLCHECK_H_
#define ARCANUM_GAME_WALLCHECK_H_

#include "game/context.h"

bool wallcheck_init(GameInitInfo* init_info);
void wallcheck_reset();
void wallcheck_exit();
void wallcheck_ping(tig_timestamp_t timestamp);
void wallcheck_recalc(int64_t loc);
void wallcheck_flush();
void wallcheck_set_enabled(bool enabled);
bool wallcheck_is_enabled();
void wallcheck_notify_player_moved(int64_t obj);

#endif /* ARCANUM_GAME_WALLCHECK_H_ */
