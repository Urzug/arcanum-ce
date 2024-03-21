#include "tig/button.h"

#include "tig/art.h"
#include "tig/debug.h"
#include "tig/menu.h"
#include "tig/rect.h"
#include "tig/sound.h"
#include "tig/window.h"

/// The maximum number of buttons.
#define MAX_BUTTONS 400

typedef enum TigButtonUsage {
    TIG_BUTTON_USAGE_FREE = 1 << 0,
    TIG_BUTTON_USAGE_RADIO = 1 << 1,
    TIG_BUTTON_USAGE_FORCE_HIDDEN = 1 << 2,
} TigButtonUsage;

typedef struct TigButton {
    /* 0000 */ unsigned int usage;
    /* 0004 */ int flags;
    /* 0008 */ tig_window_handle_t window_handle;
    /* 000C */ TigRect rect;
    /* 001C */ tig_art_id_t art_id;
    /* 0020 */ int state;
    /* 0024 */ int field_24;
    /* 0028 */ int field_28;
    /* 002C */ int field_2C;
    /* 0030 */ int field_30;
    /* 0034 */ int group;
} TigButton;

static_assert(sizeof(TigButton) == 0x38, "wrong size");

static int tig_button_free_index();
static int tig_button_handle_to_index(tig_button_handle_t button_handle);
static tig_button_handle_t tig_button_index_to_handle(int index);
static void tig_button_play_sound(tig_button_handle_t button_handle, int event);
static tig_button_handle_t tig_button_radio_group_get_selected(int group);
static void sub_5387B0(int group);
static void sub_5387D0();

// 0x5C26F8
static tig_button_handle_t dword_5C26F8 = TIG_BUTTON_HANDLE_INVALID;

// 0x5C26FC
static tig_button_handle_t dword_5C26FC = TIG_BUTTON_HANDLE_INVALID;

// 0x630D60
static TigButton buttons[MAX_BUTTONS];

// 0x6364E0
static bool busy;

// 0x537B00
int tig_button_init(TigContext* ctx)
{
    (void)ctx;

    int button_index;

    for (button_index = 0; button_index < MAX_BUTTONS; button_index++) {
        buttons[button_index].usage = TIG_BUTTON_USAGE_FREE;
    }

    return TIG_OK;
}

// 0x537B20
void tig_button_exit()
{
    int button_index;
    tig_button_handle_t button_handle;

    for (button_index = 0; button_index < MAX_BUTTONS; button_index++) {
        if ((buttons[button_index].usage & TIG_BUTTON_USAGE_FREE) == 0) {
            button_handle = tig_button_index_to_handle(button_index);
            tig_button_destroy(button_handle);
        }
    }
}

// 0x537B50
int tig_button_create(TigButtonData* button_data, tig_button_handle_t* button_handle)
{
    int button_index;
    int rc;
    TigWindowData window_data;
    TigArtFrameData art_frame_data;
    TigButton* btn;

    button_index = tig_button_free_index();
    if (button_index == -1) {
        return TIG_ERR_3;
    }

    btn = &(buttons[button_index]);

    btn->window_handle = button_data->window_handle;
    btn->group = -1;

    rc = tig_window_data(button_data->window_handle, &window_data);
    if (rc != TIG_OK) {
        return rc;
    }

    btn->flags = button_data->flags;

    if ((window_data.flags & TIG_WINDOW_FLAG_HIDDEN) != 0) {
        btn->flags |= TIG_BUTTON_FLAG_HIDDEN;
        btn->usage |= TIG_BUTTON_USAGE_FORCE_HIDDEN;
    }

    if ((btn->flags & TIG_BUTTON_FLAG_0x04) != 0) {
        btn->flags |= TIG_BUTTON_FLAG_0x02;
    }

    btn->rect.x = window_data.rect.x + button_data->x;
    btn->rect.y = window_data.rect.y + button_data->y;

    if (button_data->art_id != TIG_ART_ID_INVALID) {
        rc = tig_art_frame_data(button_data->art_id, &art_frame_data);
        if (rc != TIG_OK) {
            return rc;
        }

        btn->rect.width = art_frame_data.width;
        btn->rect.height = art_frame_data.height;
    } else {
        btn->rect.width = button_data->width;
        btn->rect.height = button_data->height;
    }

    btn->art_id = button_data->art_id;

    if ((button_data->flags & TIG_BUTTON_FLAG_0x10) != 0) {
        btn->state = TIG_BUTTON_STATE_0;
    } else {
        btn->state = TIG_BUTTON_STATE_3;
    }

    btn->field_24 = button_data->field_1C;
    btn->field_28 = button_data->field_20;
    btn->field_2C = button_data->field_24;
    btn->field_30 = button_data->field_28;

    *button_handle = tig_button_index_to_handle(button_index);

    rc = tig_window_button_add(button_data->window_handle, *button_handle);
    if (rc != TIG_OK) {
        return rc;
    }

    if ((button_data->flags & TIG_BUTTON_FLAG_HIDDEN) == 0) {
        tig_window_set_needs_display_in_rect(&(btn->rect));
        tig_button_refresh_rect(btn->window_handle, &(btn->rect));
    }

    btn->usage &= ~TIG_BUTTON_USAGE_FREE;

    return TIG_OK;
}

// 0x537CE0
int tig_button_destroy(tig_button_handle_t button_handle)
{
    int button_index;
    int rc;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        tig_debug_printf("tig_button_destroy: ERROR: Attempt to reference Empty ButtonID!\n");
        return TIG_ERR_12;
    }

    button_index = tig_button_handle_to_index(button_handle);
    rc = tig_window_button_remove(buttons[button_index].window_handle, button_handle);
    if (rc != TIG_OK) {
        return rc;
    }

    if (dword_5C26F8 == button_handle || dword_5C26FC == button_handle) {
        sub_5387D0();
    }

    tig_window_set_needs_display_in_rect(&(buttons[button_index].rect));
    tig_button_refresh_rect(buttons[button_index].window_handle, &(buttons[button_index].rect));
    buttons[button_index].usage = TIG_BUTTON_USAGE_FREE;

    return TIG_OK;
}

// 0x537D70
int tig_button_data(tig_button_handle_t button_handle, TigButtonData* button_data)
{
    int button_index;
    TigWindowData window_data;
    int rc;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        tig_debug_printf("tig_button_data: ERROR: Attempt to reference Empty ButtonID!\n");
        return TIG_ERR_12;
    }

    button_index = tig_button_handle_to_index(button_handle);
    button_data->flags = buttons[button_index].flags;
    button_data->window_handle = buttons[button_index].window_handle;

    rc = tig_window_data(buttons[button_index].window_handle, &window_data);
    if (rc != TIG_OK) {
        return rc;
    }

    button_data->x = buttons[button_index].rect.x - window_data.rect.x;
    button_data->y = buttons[button_index].rect.y - window_data.rect.y;
    button_data->art_id = buttons[button_index].art_id;
    button_data->width = buttons[button_index].rect.width;
    button_data->height = buttons[button_index].rect.height;
    button_data->field_1C = buttons[button_index].field_24;
    button_data->field_20 = buttons[button_index].field_28;
    button_data->field_24 = buttons[button_index].field_2C;
    button_data->field_28 = buttons[button_index].field_30;

    return TIG_OK;
}

// 0x537E20
int tig_button_refresh_rect(int window_handle, TigRect* rect)
{
    tig_button_handle_t* window_buttons;
    int num_window_buttons;
    TigWindowData window_data;
    int rc;
    int index;
    int button_index;
    TigRect src_rect;
    TigRect dest_rect;
    unsigned int art_id;
    TigArtBlitSpec blt;

    if (busy) {
        return TIG_OK;
    }

    num_window_buttons = tig_window_button_list(window_handle, &window_buttons);

    rc = tig_window_data(window_handle, &window_data);
    if (rc != TIG_OK) {
        return rc;
    }

    if (num_window_buttons != 0) {
        busy = true;

        for (index = 0; index < num_window_buttons; index++) {
            button_index = tig_button_handle_to_index(window_buttons[index]);

            if (tig_rect_intersection(&(buttons[button_index].rect), rect, &src_rect) == TIG_OK
                && (buttons[button_index].flags & TIG_BUTTON_FLAG_HIDDEN) == 0) {
                dest_rect.width = src_rect.width;
                dest_rect.height = src_rect.height;
                dest_rect.x = src_rect.x - window_data.rect.x;
                dest_rect.y = src_rect.y - window_data.rect.y;

                src_rect.x -= buttons[button_index].rect.x;
                src_rect.y -= buttons[button_index].rect.y;

                art_id = buttons[button_index].art_id;
                if (art_id != TIG_ART_ID_INVALID) {
                    if (buttons[button_index].state != TIG_BUTTON_STATE_3) {
                        art_id = sub_502BC0(art_id);

                        if (buttons[button_index].state == TIG_BUTTON_STATE_2) {
                            art_id = sub_502BC0(art_id);
                        }
                    }

                    blt.art_id = art_id;
                    blt.src_rect = &src_rect;
                    blt.flags = 0;
                    blt.dst_rect = &dest_rect;
                    tig_window_blit_art(window_handle, &blt);
                }
            }
        }

        busy = false;
    }

    return TIG_OK;
}

// 0x537FA0
int tig_button_free_index()
{
    int button_index;

    for (button_index = 0; button_index < MAX_BUTTONS; button_index++) {
        if ((buttons[button_index].usage & TIG_BUTTON_USAGE_FREE) != 0) {
            return button_index;
        }
    }

    return -1;
}

// 0x537FC0
int tig_button_handle_to_index(tig_button_handle_t button_handle)
{
    return (int)button_handle;
}

// 0x537FD0
tig_button_handle_t tig_button_index_to_handle(int index)
{
    return (tig_button_handle_t)index;
}

// 0x537FE0
void tig_button_state_change(tig_button_handle_t button_handle, int state)
{
    // 0x6364E4
    static bool in_state_change;

    int button_index;
    int old_state;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        tig_debug_printf("tig_button_state_change: ERROR: Attempt to reference Empty ButtonID!\n");
        return;
    }

    button_index = tig_button_handle_to_index(button_handle);
    old_state = buttons[button_index].state;

    if (old_state == TIG_BUTTON_STATE_0) {
        if (state != TIG_BUTTON_STATE_1) {
            return;
        }
        state = TIG_BUTTON_STATE_3;
    } else {
        if (state == TIG_BUTTON_STATE_1) {
            state = TIG_BUTTON_STATE_3;
        }
    }

    if (old_state != state) {
        if (!in_state_change) {
            if (state == 0) {
                in_state_change = true;
                if ((buttons[button_index].usage & TIG_BUTTON_USAGE_RADIO) != 0) {
                    sub_5387B0(buttons[button_index].group);
                }
                in_state_change = false;
            }
        }

        buttons[button_index].state = state;
        tig_button_refresh_rect(buttons[button_index].window_handle, &(buttons[button_index].rect));
        tig_window_set_needs_display_in_rect(&(buttons[button_index].rect));
    }
}

// 0x5380A0
int tig_button_state_get(tig_button_handle_t button_handle, int* state)
{
    int button_index;

    if (button_handle != TIG_BUTTON_HANDLE_INVALID) {
        tig_debug_printf("tig_button_state_get: ERROR: Attempt to reference Empty ButtonID!\n");
        return TIG_ERR_12;
    }

    button_index = tig_button_handle_to_index(button_handle);
    if (state == NULL) {
        return TIG_ERR_12;
    }

    if ((buttons[button_index].usage & TIG_BUTTON_USAGE_FREE) != 0) {
        return TIG_ERR_12;
    }

    *state = buttons[button_index].state;

    return TIG_OK;
}

// 0x5380F0
tig_button_handle_t tig_button_get_at_position(int x, int y)
{
    TigArtData art_data;
    TigButton* btn;
    tig_window_handle_t window_handle;
    tig_button_handle_t* window_buttons;
    int num_window_buttons;
    int index;
    int button_index;
    unsigned int color;

    if (tig_window_get_at_position(x, y, &window_handle) != TIG_OK) {
        return TIG_BUTTON_HANDLE_INVALID;
    }

    num_window_buttons = tig_window_button_list(window_handle, &window_buttons);

    for (index = 0; index < num_window_buttons; index++) {
        button_index = tig_button_handle_to_index(window_buttons[index]);
        btn = &(buttons[button_index]);

        if ((btn->flags & TIG_BUTTON_FLAG_HIDDEN) == 0
            && x >= btn->rect.x
            && y >= btn->rect.y
            && x < btn->rect.x + btn->rect.width
            && y < btn->rect.y + btn->rect.height) {
            if (btn->art_id == TIG_ART_ID_INVALID) {
                return window_buttons[index];
            }

            if (sub_502E50(btn->art_id, x - btn->rect.x, y - btn->rect.y, &color) == TIG_OK
                && tig_art_data(btn->art_id, &art_data) == TIG_OK
                && color != art_data.color_key) {
                return window_buttons[index];
            }
        }
    }

    return TIG_BUTTON_HANDLE_INVALID;
}

// 0x538220
bool tig_button_process_mouse_msg(TigMouseMessageData* mouse)
{
    TigMessage msg;
    tig_button_handle_t button_handle;
    int button_index;

    if (mouse->event != TIG_MOUSE_MESSAGE_LEFT_BUTTON_DOWN
        && mouse->event != TIG_MOUSE_MESSAGE_LEFT_BUTTON_UP
        && mouse->event != TIG_MOUSE_MESSAGE_MOVE) {
        return false;
    }

    msg.type = TIG_MESSAGE_TYPE_BUTTON;
    msg.data.button.x = mouse->x;
    msg.data.button.y = mouse->y;

    button_handle = tig_button_get_at_position(mouse->x, mouse->y);
    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        if (dword_5C26FC != TIG_BUTTON_HANDLE_INVALID) {
            tig_button_state_change(dword_5C26FC, TIG_BUTTON_STATE_3);

            tig_timer_start(&(msg.time));
            msg.data.button.button_handle = dword_5C26FC;
            msg.data.button.state = TIG_BUTTON_STATE_3;
            tig_message_enqueue(&msg);

            tig_button_play_sound(dword_5C26FC, TIG_BUTTON_STATE_3);

            dword_5C26FC = TIG_BUTTON_HANDLE_INVALID;
        }

        if (dword_5C26F8 != TIG_BUTTON_HANDLE_INVALID) {
            if (mouse->event == TIG_MOUSE_MESSAGE_MOVE) {
                tig_button_state_change(dword_5C26F8, TIG_BUTTON_STATE_1);
            } else if (mouse->event == TIG_MOUSE_MESSAGE_LEFT_BUTTON_UP) {
                dword_5C26F8 = TIG_BUTTON_HANDLE_INVALID;
            }
        }

        return false;
    }

    if (dword_5C26F8 != TIG_BUTTON_HANDLE_INVALID && mouse->event == TIG_MOUSE_MESSAGE_MOVE) {
        if (dword_5C26F8 == button_handle) {
            tig_button_state_change(button_handle, TIG_BUTTON_STATE_0);

            if (dword_5C26FC != button_handle) {
                tig_timer_start(&(msg.time));
                msg.data.button.button_handle = button_handle;
                msg.data.button.state = TIG_BUTTON_STATE_2;
                tig_message_enqueue(&msg);

                tig_button_play_sound(button_handle, TIG_BUTTON_STATE_2);

                dword_5C26FC = button_handle;
            }
        } else {
            tig_button_state_change(dword_5C26F8, TIG_BUTTON_STATE_1);
        }

        return false;
    }

    if (mouse->event == TIG_MOUSE_MESSAGE_LEFT_BUTTON_DOWN) {
        int new_state;

        button_index = tig_button_handle_to_index(button_handle);

        if ((buttons[button_index].flags & TIG_BUTTON_MENU_BAR) != 0) {
            // Let menu system handle this click.
            tig_menu_bar_on_click(button_handle);
            return true;
        }

        new_state = TIG_BUTTON_STATE_0;
        if (dword_5C26F8 == TIG_BUTTON_HANDLE_INVALID) {
            if ((buttons[button_index].flags & TIG_BUTTON_FLAG_0x02) != 0) {
                if (buttons[button_index].state == TIG_BUTTON_STATE_0
                    && (buttons[button_index].flags & TIG_BUTTON_FLAG_0x04) != 0) {
                    return true;
                }

                if ((buttons[button_index].usage & TIG_BUTTON_USAGE_RADIO) != 0) {
                    sub_5387B0(buttons[button_index].group);
                }

                new_state = buttons[button_index].state == TIG_BUTTON_STATE_0
                    ? TIG_BUTTON_STATE_1
                    : TIG_BUTTON_STATE_0;
                tig_button_state_change(button_handle, new_state);

                dword_5C26FC = TIG_BUTTON_HANDLE_INVALID;
            } else {
                dword_5C26F8 = button_handle;
                tig_button_state_change(button_handle, TIG_BUTTON_STATE_0);
            }

            if (dword_5C26F8 == TIG_BUTTON_HANDLE_INVALID) {
                tig_timer_start(&(msg.time));
                msg.data.button.button_handle = button_handle;
                msg.data.button.state = new_state;
                tig_message_enqueue(&msg);

                tig_button_play_sound(button_handle, new_state);

                return true;
            }
        }

        if (dword_5C26F8 == button_handle) {
            tig_timer_start(&(msg.time));
            msg.data.button.button_handle = button_handle;
            msg.data.button.state = new_state;
            tig_message_enqueue(&msg);

            tig_button_play_sound(button_handle, new_state);

            return true;
        }

        return false;
    }

    if (mouse->event == TIG_MOUSE_MESSAGE_LEFT_BUTTON_UP) {
        if (dword_5C26F8 != button_handle) {
            dword_5C26F8 = TIG_BUTTON_HANDLE_INVALID;
            return false;
        }

        button_index = tig_button_handle_to_index(button_handle);

        if ((buttons[button_index].flags & TIG_BUTTON_FLAG_0x02) == 0) {
            dword_5C26F8 = TIG_BUTTON_HANDLE_INVALID;
            dword_5C26FC = button_handle;
            tig_button_state_change(button_handle, TIG_BUTTON_STATE_1);
            tig_button_state_change(button_handle, TIG_BUTTON_STATE_2);

            tig_timer_start(&(msg.time));
            msg.data.button.button_handle = button_handle;
            msg.data.button.state = TIG_BUTTON_STATE_1;
            tig_message_enqueue(&msg);

            tig_button_play_sound(button_handle, TIG_BUTTON_STATE_1);
            return true;
        }

        return false;
    }

    if (dword_5C26FC != button_handle
        && dword_5C26F8 == TIG_BUTTON_HANDLE_INVALID
        && mouse->event == TIG_MOUSE_MESSAGE_MOVE) {
        button_index = tig_button_handle_to_index(button_handle);

        if (dword_5C26FC != TIG_BUTTON_HANDLE_INVALID) {
            if ((buttons[button_index].flags & TIG_BUTTON_FLAG_0x02) == 0
                || buttons[button_index].state != TIG_BUTTON_STATE_0) {
                tig_button_state_change(dword_5C26FC, TIG_BUTTON_STATE_3);
            }

            tig_timer_start(&(msg.time));
            msg.data.button.button_handle = dword_5C26FC;
            msg.data.button.state = TIG_BUTTON_STATE_3;
            tig_message_enqueue(&msg);

            tig_button_play_sound(dword_5C26FC, TIG_BUTTON_STATE_3);
        }

        dword_5C26FC = button_handle;

        if ((buttons[button_index].flags & TIG_BUTTON_FLAG_0x02) == 0
            || buttons[button_index].state != TIG_BUTTON_STATE_0) {
            tig_button_state_change(dword_5C26FC, TIG_BUTTON_STATE_2);
        }

        tig_timer_start(&(msg.time));
        msg.data.button.button_handle = dword_5C26FC;
        msg.data.button.state = TIG_BUTTON_STATE_2;
        tig_message_enqueue(&msg);

        tig_button_play_sound(dword_5C26FC, TIG_BUTTON_STATE_2);

        return false;
    }

    return false;
}

// 0x5385C0
void tig_button_play_sound(tig_button_handle_t button_handle, int event)
{
    TigButton* btn;
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        // FIXME: Wrong function name.
        tig_debug_printf("tig_button_state_change: ERROR: Attempt to reference Empty ButtonID!\n");
        return;
    }

    button_index = tig_button_handle_to_index(button_handle);
    btn = &(buttons[button_index]);

    switch (event) {
    case TIG_BUTTON_STATE_0:
        tig_sound_quick_play(btn->field_24);
        break;
    case TIG_BUTTON_STATE_1:
        tig_sound_quick_play(btn->field_28);
        break;
    case TIG_BUTTON_STATE_2:
        tig_sound_quick_play(btn->field_2C);
        break;
    case TIG_BUTTON_STATE_3:
        tig_sound_quick_play(btn->field_30);
        break;
    }
}

// 0x538670
int tig_button_radio_group_create(int count, tig_button_handle_t* button_handles, int selected)
{
    int index;
    int group;
    int button_index;
    TigButton* btn;

    if (!(selected >= 0 && selected < count)) {
        return TIG_ERR_12;
    }

    // Find new group identifier.
    group = 0;
    for (button_index = 0; button_index < MAX_BUTTONS; button_index++) {
        btn = &(buttons[button_index]);
        if ((btn->usage & TIG_BUTTON_USAGE_FREE) == 0
            && (btn->usage & TIG_BUTTON_USAGE_RADIO) != 0
            && btn->group > group) {
            group = btn->group + 1;
        }
    }

    for (index = 0; index < count; index++) {
        button_index = tig_button_handle_to_index(button_handles[index]);
        btn = &(buttons[button_index]);

        if ((btn->usage & TIG_BUTTON_USAGE_FREE) != 0) {
            return TIG_ERR_12;
        }

        btn->usage |= TIG_BUTTON_USAGE_RADIO;
        btn->flags |= TIG_BUTTON_FLAG_0x02;
        btn->flags |= TIG_BUTTON_FLAG_0x04;
        btn->group = group;
    }

    tig_button_state_change(button_handles[selected], 0);

    return TIG_OK;
}

// 0x538730
tig_button_handle_t sub_538730(tig_button_handle_t button_handle)
{
    int button_index = tig_button_handle_to_index(button_handle);
    return tig_button_radio_group_get_selected(buttons[button_index].group);
}

// 0x538760
tig_button_handle_t tig_button_radio_group_get_selected(int group)
{
    int index;
    TigButton* btn;

    for (index = 0; index < MAX_BUTTONS; index++) {
        btn = &(buttons[index]);
        if ((btn->usage & TIG_BUTTON_USAGE_FREE) == 0
            && (btn->usage & TIG_BUTTON_USAGE_RADIO) != 0
            && btn->group == group
            && btn->state == TIG_BUTTON_STATE_0) {
            return tig_button_index_to_handle(index);
        }
    }

    return TIG_BUTTON_HANDLE_INVALID;
}

// 0x5387B0
void sub_5387B0(int group)
{
    tig_button_handle_t button_handle = tig_button_radio_group_get_selected(group);
    if (button_handle != TIG_BUTTON_HANDLE_INVALID) {
        tig_button_state_change(button_handle, TIG_BUTTON_STATE_1);
    }
}

// 0x5387D0
void sub_5387D0()
{
    dword_5C26F8 = TIG_BUTTON_HANDLE_INVALID;
    dword_5C26FC = TIG_BUTTON_HANDLE_INVALID;
}

// 0x5387E0
int tig_button_show(tig_button_handle_t button_handle)
{
    TigButton* btn;
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        return TIG_OK;
    }

    button_index = tig_button_handle_to_index(button_handle);
    btn = &(buttons[button_index]);

    if ((btn->flags & TIG_BUTTON_FLAG_HIDDEN) == 0) {
        return TIG_ERR_12;
    }

    btn->flags &= ~TIG_BUTTON_FLAG_HIDDEN;

    tig_button_refresh_rect(btn->window_handle, &(btn->rect));
    tig_window_set_needs_display_in_rect(&(btn->rect));

    return TIG_OK;
}

// 0x538840
int tig_button_hide(tig_button_handle_t button_handle)
{
    TigButton* btn;
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        return TIG_OK;
    }

    button_index = tig_button_handle_to_index(button_handle);
    btn = &(buttons[button_index]);

    if ((btn->flags & TIG_BUTTON_FLAG_HIDDEN) != 0) {
        if ((btn->usage & TIG_BUTTON_USAGE_FORCE_HIDDEN) != 0) {
            btn->usage &= ~TIG_BUTTON_USAGE_FORCE_HIDDEN;
        }
        return TIG_ERR_12;
    }

    btn->flags |= TIG_BUTTON_FLAG_HIDDEN;

    tig_button_refresh_rect(btn->window_handle, &(btn->rect));
    tig_window_set_needs_display_in_rect(&(btn->rect));

    return TIG_OK;
}

// 0x5388B0
int tig_button_is_hidden(tig_button_handle_t button_handle, bool* hidden)
{
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        return TIG_ERR_12;
    }

    if (hidden == NULL) {
        return TIG_ERR_12;
    }

    button_index = tig_button_handle_to_index(button_handle);
    *hidden = (buttons[button_index].flags & TIG_BUTTON_FLAG_HIDDEN) != 0;

    return TIG_OK;
}

// 0x538900
int tig_button_show_force(tig_button_handle_t button_handle)
{
    TigButton* btn;
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        return TIG_OK;
    }

    button_index = tig_button_handle_to_index(button_handle);
    btn = &(buttons[button_index]);

    if ((btn->flags & TIG_BUTTON_FLAG_HIDDEN) == 0) {
        return TIG_ERR_12;
    }

    if ((btn->usage & TIG_BUTTON_USAGE_FORCE_HIDDEN) == 0) {
        return TIG_ERR_12;
    }

    btn->flags &= ~TIG_BUTTON_FLAG_HIDDEN;
    btn->usage &= ~TIG_BUTTON_USAGE_FORCE_HIDDEN;

    tig_button_refresh_rect(btn->window_handle, &(btn->rect));
    tig_window_set_needs_display_in_rect(&(btn->rect));

    return TIG_OK;
}

// 0x538980
int tig_button_hide_force(tig_button_handle_t button_handle)
{
    TigButton* btn;
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        return TIG_OK;
    }

    button_index = tig_button_handle_to_index(button_handle);
    btn = &(buttons[button_index]);

    if ((btn->flags & TIG_BUTTON_FLAG_HIDDEN) != 0) {
        return TIG_ERR_12;
    }

    btn->flags |= TIG_BUTTON_FLAG_HIDDEN;
    btn->usage |= TIG_BUTTON_USAGE_FORCE_HIDDEN;

    tig_button_refresh_rect(btn->window_handle, &(btn->rect));
    tig_window_set_needs_display_in_rect(&(btn->rect));

    return TIG_OK;
}

// 0x5389F0
void tig_button_set_art(tig_button_handle_t button_handle, tig_art_id_t art_id)
{
    TigArtFrameData art_frame_data;
    TigButton* btn;
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        return;
    }

    button_index = tig_button_handle_to_index(button_handle);
    btn = &(buttons[button_index]);

    if (art_id != TIG_ART_ID_INVALID) {
        tig_art_frame_data(art_id, &art_frame_data);

        btn->rect.width = art_frame_data.width;
        btn->rect.height = art_frame_data.height;
    }

    btn->art_id = art_id;

    if ((btn->flags & TIG_BUTTON_FLAG_HIDDEN) == 0) {
        tig_window_set_needs_display_in_rect(&(btn->rect));
        tig_button_refresh_rect(btn->window_handle, &(btn->rect));
    }
}
