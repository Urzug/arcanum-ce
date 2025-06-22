#ifndef ARCANUM_UI_SCHEMATIC_UI_H_
#define ARCANUM_UI_SCHEMATIC_UI_H_

#include "game/context.h"
#include "game/schematic.h"

bool schematic_ui_init(GameInitInfo* init_info);
void schematic_ui_exit();
void schematic_ui_reset();
void schematic_ui_toggle(int64_t primary_obj, int64_t secondary_obj);
void schematic_ui_close();
void schematic_ui_info_get(int schematic, SchematicInfo* schematic_info);
bool schematic_ui_process(int schematic, int64_t primary_obj, int64_t secondary_obj);
bool schematic_ui_feedback(bool success, int64_t primary_obj, int64_t secondary_obj);
char* sub_56E9D0(int schematic);
char* schematic_ui_product_get(int tech, int degree);
void schematic_ui_components_get(int tech, int degree, char* item1, char* item2);

#endif /* ARCANUM_UI_SCHEMATIC_UI_H_ */
