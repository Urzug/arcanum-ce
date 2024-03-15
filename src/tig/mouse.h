#ifndef ARCANUM_TIG_MOUSE_H_
#define ARCANUM_TIG_MOUSE_H_

#include <stdbool.h>

#include "tig/rect.h"
#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum TigMouseButton {
    TIG_MOUSE_BUTTON_LEFT,
    TIG_MOUSE_BUTTON_RIGHT,
    TIG_MOUSE_BUTTON_MIDDLE,
    TIG_MOUSE_BUTTON_COUNT,
} TigMouseButton;

typedef enum TigMouseStateFlags {
    TIG_MOUSE_STATE_HIDDEN = 0x001,
    TIG_MOUSE_STATE_LEFT_MOUSE_DOWN = 0x002,
    TIG_MOUSE_STATE_LEFT_MOUSE_DOWN_REPEAT = 0x004,
    TIG_MOUSE_STATE_LEFT_MOUSE_UP = 0x008,
    TIG_MOUSE_STATE_RIGHT_MOUSE_DOWN = 0x010,
    TIG_MOUSE_STATE_RIGHT_MOUSE_DOWN_REPEAT = 0x020,
    TIG_MOUSE_STATE_RIGHT_MOUSE_UP = 0x040,
    TIG_MOUSE_STATE_MIDDLE_MOUSE_DOWN = 0x080,
    TIG_MOUSE_STATE_MIDDLE_MOUSE_DOWN_REPEAT = 0x100,
    TIG_MOUSE_STATE_MIDDLE_MOUSE_UP = 0x200,
} TigMouseStateFlags;

typedef struct TigMouseState {
    unsigned int flags;
    TigRect frame;
    int offset_x;
    int offset_y;
    int x;
    int y;
    int z;
} TigMouseState;

/// @see 0x4FF9F0.
static_assert(sizeof(TigMouseState) == 0x28, "wrong size");

int tig_mouse_init(TigContext* ctx);
void tig_mouse_exit();
void tig_mouse_set_active(bool is_active);
bool tig_mouse_ping();

/// Forwards "mouse move" event from Windows Messages to mouse system.
///
/// This function is only used from WNDPROC and should not be called in software
/// cursor mode (managed by DirectInput).
void tig_mouse_set_position(int x, int y, int z);

/// Forwards "mouse down" and "mouse up" events from Windows Messages to mouse
/// system.
///
/// This function is only used from WNDPROC and should not be called in software
/// cursor mode (managed by DirectInput).
void tig_mouse_set_button(int button, bool pressed);

/// Obtains current mouse state and returns `TIG_OK`.
///
/// Returns `TIG_NOT_INITIALIZED` if mouse system was not initialized.
int tig_mouse_get_state(TigMouseState* mouse_state);

/// Hides mouse cursor.
int tig_mouse_hide();

/// Shows mouse cursor.
int tig_mouse_show();

/// Renders mouse cursor to screen.
void tig_mouse_display();

/// Refreshes internally managed cursor surface.
///
/// This function does nothing in hardware cursor mode.
void tig_mouse_cursor_refresh();

/// Sets mouse cursor art.
///
/// This function does nothing in hardware cursor mode.
int tig_mouse_cursor_set_art_id(unsigned int art_id);

/// Sets art offset from the real cursor position.
void tig_mouse_cursor_set_offset(int x, int y);

/// Returns art id of mouse cursor.
///
/// In hardware cursor mode the result is undefined.
unsigned int tig_mouse_cursor_get_art_id();

/// Adds overlay to the mouse cursor.
int tig_mouse_cursor_overlay(unsigned int art_id, int x, int y);

int sub_500560();
void sub_500570();
void tig_mouse_set_z_axis_enabled(bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* ARCANUM_TIG_MOUSE_H_ */
