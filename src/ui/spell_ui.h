#ifndef ARCANUM_UI_SPELL_UI_H_
#define ARCANUM_UI_SPELL_UI_H_

#include "game/context.h"
#include "game/target.h"

typedef enum SpellUiActivate {
    SPELL_UI_ACTIVATE_OK,
    SPELL_UI_ACTIVATE_ERR,
} SpellUiActivate;

bool spell_ui_init(GameInitInfo* init_info);
void spell_ui_reset();
void spell_ui_exit();
bool spell_ui_save(TigFile* stream);
bool spell_ui_load(GameLoadInfo* load_info);
void spell_ui_error_target_not_damaged();
SpellUiActivate spell_ui_activate(int64_t obj, int spl);
void sub_57BFF0();
void sub_57C040(int64_t obj, int index);
void sub_57C080(int64_t obj, int spl);
void sub_57C0B0();
void sub_57C0E0();
void sub_57C110(S4F2810* a1);
bool spell_ui_maintain_add(int mt_id);
void spell_ui_maintain_end(int mt_id);
void spell_ui_maintain_click(int slot);
void spell_ui_maintain_hover(int slot);
void spell_ui_maintain_refresh();
bool spell_ui_maintain_has(int slot);
void spell_ui_add(int64_t obj, int spell);
void spell_ui_remove(int64_t obj, int spell);

#endif /* ARCANUM_UI_SPELL_UI_H_ */
