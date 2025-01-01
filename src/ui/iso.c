#include "ui/iso.h"

#include "game/gamelib.h"
#include "ui/intgame.h"

static void sub_557F30();

// 0x64C6F8
static TigRect stru_64C6F8;

// 0x64C708
static bool iso_interface_created;

// 0x64C70C
static tig_font_handle_t dword_64C70C;

// 0x64C710
static int dword_64C710;

// 0x64C714
static tig_window_handle_t iso_window_handle;

// 0x64C718
static TigRect iso_content_rect;

// 0x557DF0
bool iso_init(GameInitInfo* init_info)
{
    TigWindowData window_data;

    iso_interface_created = false;
    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    iso_window_handle = init_info->iso_window_handle;

    iso_content_rect.x = 0;
    iso_content_rect.y = 0;
    iso_content_rect.width = window_data.rect.width;
    iso_content_rect.height = window_data.rect.height;
    sub_558130(&iso_content_rect);

    iso_interface_create(iso_window_handle);
    iso_interface_created = true;

    dword_64C710 = 0;
    sub_557F30();

    return true;
}

// 0x557EA0
void iso_exit()
{
    if (iso_interface_created) {
        iso_interface_destroy();
    }
    tig_font_destroy(dword_64C70C);
}

// 0x557EC0
void iso_reset()
{
    sub_54AA30();
    sub_557FD0(NULL);
}

// 0x557ED0
void iso_resize(GameResizeInfo* resize_info)
{
    iso_window_handle = resize_info->window_handle;
    iso_content_rect = resize_info->content_rect;
    sub_5581A0(&iso_content_rect);
}

// 0x557F30
void sub_557F30()
{
    TigFont font;

    font.flags = 0;
    tig_art_interface_id_create(327, 0, 0, 0, &(font.art_id));
    font.str = NULL;
    font.color = tig_color_make(255, 255, 255);
    tig_font_create(&font, &dword_64C70C);
}

// 0x557FD0
void sub_557FD0(const char* str)
{
    TigRect rect;
    TigWindowData window_data;
    TigFont font;

    rect = iso_content_rect;
    if (tig_window_data(iso_window_handle, &window_data) == TIG_OK) {
        rect = window_data.rect;
    }

    tig_window_fill(iso_window_handle, &rect, tig_color_make(0, 0, 0));

    if (str != NULL && *str != '\0') {
        tig_font_push(dword_64C70C);

        font.width = 0;
        font.str = str;
        sub_535390(&font);

        rect.x = (rect.width - font.width) / 2;
        rect.y = (rect.height - font.height) / 2;
        rect.width = font.width;
        rect.height = font.height;
        tig_window_text_write(iso_window_handle, str, &rect);

        tig_font_pop();
    }
}

// 0x558110
bool iso_redraw()
{
    bool ret = sub_402E50();
    sub_551080();
    return ret;
}

// 0x558130
void sub_558130(TigRect* rect)
{
    if (rect != NULL) {
        stru_64C6F8 = *rect;
    } else {
        stru_64C6F8 = iso_content_rect;
        sub_5581A0(&iso_content_rect);
    }
}

// 0x5581A0
void sub_5581A0(TigRect* rect)
{
    sub_402D30(rect);
}
