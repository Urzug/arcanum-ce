#ifndef ARCANUM_GAME_REACTION_H_
#define ARCANUM_GAME_REACTION_H_

#include "game/context.h"

typedef enum Reaction {
    REACTION_LOVE,
    REACTION_AMIABLE,
    REACTION_COURTEOUS,
    REACTION_NEUTRAL,
    REACTION_SUSPICIOUS,
    REACTION_DISLIKE,
    REACTION_HATRED,
    REACTION_COUNT,
} Reaction;

bool reaction_init(GameInitInfo* init_info);
void reaction_exit();
bool reaction_met_before(int64_t npc_obj, int64_t pc_obj);
int reaction_get(int64_t npc_obj, int64_t pc_obj);
int GetReactionScore(int64_t npc_obj, int64_t pc_obj);
void reaction_adj(int64_t npc_obj, int64_t pc_obj, int value);
void sub_4C0F50(int64_t a1, int64_t a2);
int reaction_translate(int value);
const char* reaction_get_name(int reaction);
void ai_enter_dialog(int64_t pc, int64_t npc);
void ai_exit_dialog(int64_t pc, int64_t npc);
int64_t GetPCWithHighestReaction(int64_t npc);
int barter_get_haggled_price(int64_t a1, int64_t a2, int a3);
void sub_4C11D0(int64_t a1, int64_t a2, int a3);

#endif /* ARCANUM_GAME_REACTION_H_ */
