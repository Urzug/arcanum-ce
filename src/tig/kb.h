#ifndef ARCANUM_TIG_KB_H_
#define ARCANUM_TIG_KB_H_

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int dword_62B1A8[16];

int tig_kb_init(TigContext* ctx);
void tig_kb_exit();
bool tig_kb_is_key_pressed(int key);
bool tig_kb_get_modifier(int key);
void tig_kb_ping();

#ifdef __cplusplus
}
#endif

#endif /* ARCANUM_TIG_KB_H_ */
