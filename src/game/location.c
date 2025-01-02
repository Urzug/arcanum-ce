#include "game/location.h"

#include "game/path.h"
#include "game/target.h"

// 0x5FC278
static TigRect location_iso_content_rect;

// 0x5FC288
static int64_t location_limit_y;

// 0x5FC290
static int64_t location_screen_center_y;

// 0x5FC298
static IsoInvalidateRectFunc* location_iso_invalidate_rect;

// 0x5FC2A0
static int64_t location_screen_center_x;

// 0x5FC2A8
static int8_t byte_5FC2A8[30];

// 0x5FC2C8
static bool location_editor;

// 0x5FC2CC
static int location_iso_window_handle;

// 0x5FC2D0
static ViewOptions location_view_options;

// 0x5FC2D8
static int64_t qword_5FC2D8;

// 0x5FC2E0
static int64_t location_origin_x;

// 0x5FC2E8
static int64_t location_origin_y;

// 0x5FC2F0
static int64_t location_limit_x;

// 0x5FC2F8
static LocationFunc5FC2F8* dword_5FC2F8;

// 0x4B8440
bool location_init(GameInitInfo* init_info)
{
    TigWindowData window_data;

    if (tig_window_data(init_info->iso_window_handle, &window_data)) {
        tig_debug_printf("location_init: ERROR: couldn't grab window data!\n");
        exit(EXIT_SUCCESS);
    }

    location_iso_window_handle = init_info->iso_window_handle;
    location_iso_invalidate_rect = init_info->invalidate_rect_func;

    location_iso_content_rect.x = 0;
    location_iso_content_rect.y = 0;
    location_iso_content_rect.width = window_data.rect.width;
    location_iso_content_rect.height = window_data.rect.height;

    location_origin_x = 0;
    location_origin_y = 0;

    location_screen_center_x = window_data.rect.width / 2;
    location_screen_center_y = window_data.rect.height / 2;

    location_limits_set(0x100000000, 0x100000000);
    location_origin_set(sub_4B9810());

    location_view_options.type = VIEW_TYPE_ISOMETRIC;
    location_editor = init_info->editor;

    return true;
}

// 0x4B8530
void location_exit()
{
}

// 0x4B8540
void location_resize(GameResizeInfo* resize_info)
{
    location_iso_content_rect = resize_info->content_rect;
    location_iso_window_handle = resize_info->window_handle;
    location_screen_center_x = location_iso_content_rect.width / 2;
    location_screen_center_y = location_iso_content_rect.height / 2;
}

// 0x4B85A0
bool location_update_view(ViewOptions* view_options)
{
    int64_t loc;

    if (!location_at(location_screen_center_x, location_screen_center_y, &loc)) {
        return false;
    }

    if (view_options->type == location_view_options.type) {
        if (view_options->type == VIEW_TYPE_ISOMETRIC) {
            return true;
        }
        if (view_options->zoom == location_view_options.zoom) {
            return true;
        }
    }

    if (view_options->type == VIEW_TYPE_ISOMETRIC) {
        location_origin_x = 0;
        location_origin_y = 0;
        location_view_options = *view_options;
        location_origin_set(loc);
        return true;
    }

    if (view_options->zoom >= 12 && view_options->zoom <= 64) {
        location_origin_x = location_iso_content_rect.width;
        location_origin_y = 0;
        location_view_options = *view_options;
        location_origin_set(loc);
        return true;
    }

    return false;
}

// 0x4B8680
void location_xy(int64_t loc, int64_t* sx, int64_t* sy)
{
    if (location_view_options.type == VIEW_TYPE_ISOMETRIC) {
        *sx = location_origin_x + 40 * (LOCATION_GET_Y(loc) - LOCATION_GET_X(loc) - 1);
        *sy = location_origin_y + 20 * (LOCATION_GET_Y(loc) + LOCATION_GET_X(loc));
    } else {
        *sx = location_origin_x - location_view_options.zoom * LOCATION_GET_X(loc);
        *sy = location_origin_y + location_view_options.zoom * LOCATION_GET_Y(loc);
    }
}

// 0x4B8730
bool location_at(int64_t sx, int64_t sy, int64_t* loc_ptr)
{
    int64_t new_x;
    int64_t new_y;
    int64_t dx;
    int64_t dy;

    if (location_view_options.type == VIEW_TYPE_ISOMETRIC) {
        dy = sy - location_origin_y;
        dx = (sx - location_origin_x) >> 1;

        if (dy - dx < INT_MIN) {
            return false;
        }

        new_x = (dy - dx) / 40;
        if (dy - dx < 0) {
            new_x--;
        }

        if (new_x < 0 || new_x >= location_limit_x) {
            return false;
        }

        if (dy + dx > INT_MAX) {
            return false;
        }

        new_y = (dy + dx) / 40;
        if (dy + dx < 0) {
            new_y--;
        }

        if (new_y < 0 || new_y >= location_limit_y) {
            return false;
        }

        *loc_ptr = LOCATION_MAKE(new_x, new_y);
    } else {
        new_x = (location_origin_x + location_view_options.zoom - sx - 1) / location_view_options.zoom;
        if (new_x < 0 || new_x >= location_limit_x) {
            return false;
        }

        new_y = (sy - location_origin_y) / location_view_options.zoom;
        if (new_y < 0 || new_y >= location_limit_y) {
            return false;
        }

        *loc_ptr = LOCATION_MAKE(new_x, new_y);
    }

    return true;
}

// 0x4B8940
void sub_4B8940(int64_t location, int64_t* x, int64_t* y)
{
    int64_t saved_x;
    int64_t saved_y;
    int64_t x1;
    int64_t y1;
    int64_t x2;
    int64_t y2;

    if (location_view_options.type == VIEW_TYPE_ISOMETRIC) {
        saved_x = location_origin_x;
        saved_y = location_origin_y;

        location_origin_x = 0;
        location_origin_y = 0;
        location_xy(0, &x1, &y1);
        location_xy(location, &x2, &y2);
        *x = x1 + location_iso_content_rect.width / 2 - x2 - saved_x;
        *y = y1 + location_iso_content_rect.height / 2 - y2 - saved_y;

        location_origin_x = saved_x;
        location_origin_y = saved_y;
    } else {
        location_xy(location, &x1, &y1);
        *x = location_screen_center_x - (location_view_options.zoom / 2 + x1);
        *y = location_screen_center_y - (location_view_options.zoom / 2 + y1);
    }
}

// 0x4B8AD0
void location_origin_get(int64_t* sx, int64_t* sy)
{
    *sx = location_origin_x;
    *sy = location_origin_y;
}

// 0x4B8B30
void location_origin_scroll(int64_t dx, int64_t dy)
{
    int64_t tmp;

    if (!location_editor && location_view_options.type == VIEW_TYPE_ISOMETRIC) {
        if (dx + location_origin_x > location_screen_center_x) {
            if (dy + location_origin_y + qword_5FC2D8 > location_screen_center_y) {
                if (!location_at(-dx, -dy, &tmp)) {
                    return;
                }

                if (!location_at(-dx, location_iso_content_rect.height - dy, &tmp)) {
                    return;
                }
            }
        } else {
            if (dy + location_origin_y + qword_5FC2D8 > location_screen_center_y) {
                if (!location_at(location_iso_content_rect.width - dx, -dy, &tmp)) {
                    return;
                }

                if (!location_at(location_iso_content_rect.width - dx, location_iso_content_rect.height - dy, &tmp)) {
                    return;
                }
            }
        }
    }

    location_origin_x += dx;
    location_origin_y += dy;
}

// 0x4B8CE0
void location_origin_set(int64_t location)
{
    int64_t sx;
    int64_t sy;

    sub_4B8940(location, &sx, &sy);
    location_origin_scroll(sx, sy);

    location_iso_invalidate_rect(&location_iso_content_rect);

    if (dword_5FC2F8 != NULL) {
        dword_5FC2F8(location);
    }
}

// 0x4B8D40
void location_set_func_5FC2F8(LocationFunc5FC2F8* func)
{
    dword_5FC2F8 = func;
}

// NOTE: Original code is likely different.
//
// 0x4B8D50
int location_rot(int64_t a, int64_t b)
{
    int dx;
    int dy;
    int sx;
    int sy;
    int ddx;
    int ddy;
    int rot;

    dx = (int)location_get_x(b) - (int)location_get_x(a);
    dy = (int)location_get_y(b) - (int)location_get_y(a);

    ddx = 2 * abs(dx);
    ddy = 2 * abs(dy);

    if (dx < 0) {
        sx = -1;
    } else if (dx > 0) {
        sx = 1;
    } else {
        sx = 0;
    }

    if (dy < 0) {
        sy = -1;
    } else if (dy > 0) {
        sy = 1;
    } else {
        sy = 0;
    }

    if (ddx > ddy) {
        if (ddy - ddx / 2 < 0) {
            sy = 0;
        }
    } else {
        if (ddx - ddy / 2 < 0) {
            sx = 0;
        }
    }

    if (sy >= 0) {
        if (sx >= 0) {
            if (sy > 0) {
                if (sx > 0) {
                    rot = 4;
                } else {
                    rot = 3;
                }
            } else {
                rot = 5;
            }
        } else if (sy > 0) {
            rot = 2;
        } else {
            rot = 1;
        }
    } else if (sx > -1) {
        if (sx > 0) {
            rot = 6;
        } else {
            rot = 7;
        }
    } else {
        rot = 0;
    }

    return rot;
}

// 0x4B8FF0
bool sub_4B8FF0(int64_t loc, int direction, int64_t* new_loc_ptr)
{
    int x;
    int y;

    x = (int)location_get_x(loc);
    y = (int)location_get_y(loc);

    switch (direction) {
    case 0:
        x -= 1;
        y -= 1;
        break;
    case 1:
        x -= 1;
        break;
    case 2:
        x -= 1;
        y += 1;
        break;
    case 3:
        y += 1;
        break;
    case 4:
        x += 1;
        y += 1;
        break;
    case 5:
        x += 1;
        break;
    case 6:
        x += 1;
        y -= 1;
        break;
    case 7:
        y -= 1;
        break;
    }

    if (x < 0 || x >= location_limit_x
        || y < 0 || y >= location_limit_y) {
        return false;
    }

    *new_loc_ptr = location_make(x, y);

    return true;
}

// 0x4B90D0
bool sub_4B90D0(int64_t loc, int rot, int range, int64_t* new_loc_ptr)
{
    int dist;

    *new_loc_ptr = loc;

    for (dist = 0; dist < range; dist++) {
        if (!sub_4B8FF0(*new_loc_ptr, rot, new_loc_ptr)) {
            return false;
        }
    }

    return true;
}

// 0x4B9130
bool sub_4B9130(TigRect* rect, LocRect* loc_rect)
{
    TigRect frame;
    uint64_t tmp;

    if (rect != NULL) {
        frame = *rect;
    } else {
        frame = location_iso_content_rect;
    }

    sub_4B98B0(frame.x, frame.y, &tmp, &(loc_rect->y1));
    sub_4B98B0(frame.x + frame.width, frame.y, &(loc_rect->x1), &tmp);
    sub_4B98B0(frame.x, frame.y + frame.height, &(loc_rect->x2), &tmp);
    sub_4B98B0(frame.x + frame.width, frame.y + frame.height, &tmp, &(loc_rect->y2));

    if (loc_rect->x1 > loc_rect->x2
        || loc_rect->y1 > loc_rect->y2) {
        return false;
    }

    if (loc_rect->x1 < 0) {
        loc_rect->x1 = 0;
    } else if (loc_rect->x1 >= location_limit_x) {
        loc_rect->x1 = location_limit_x - 1;
    }

    if (loc_rect->x2 < 0) {
        loc_rect->x2 = 0;
    } else if (loc_rect->x2 >= location_limit_x) {
        loc_rect->x2 = location_limit_x - 1;
    }

    if (loc_rect->y1 < 0) {
        loc_rect->y1 = 0;
    } else if (loc_rect->y1 >= location_limit_y) {
        loc_rect->y1 = location_limit_y - 1;
    }

    if (loc_rect->y2 < 0) {
        loc_rect->y2 = 0;
    } else if (loc_rect->y2 >= location_limit_y) {
        loc_rect->y2 = location_limit_y - 1;
    }

    if (loc_rect->x1 == loc_rect->x2
        || loc_rect->y1 == loc_rect->y2) {
        return false;
    }

    return true;
}

// 0x4B93F0
void sub_4B93F0(int a1, int a2, int* a3, int* a4)
{
    int v1;
    int v2;

    v1 = (a1 - 40) / 2;
    v2 = 2 * (a2 / 2);
    *a3 = v2 - v1;
    *a4 = v1 + v2;
}

// 0x4B9420
bool sub_4B9420(int64_t* loc_ptr, int* a2, int* a3)
{
    int64_t x;
    int64_t y;
    int64_t new_loc;
    int64_t new_x;
    int64_t new_y;

    location_xy(*loc_ptr, &x, &y);
    x += *a2 + 40;
    y += *a3 + 20;

    if (!location_at(x, y, &new_loc)) {
        return false;
    }

    if (*loc_ptr == new_loc) {
        return false;
    }

    location_xy(new_loc, &new_x, &new_y);

    *a2 = (int)(x - (new_x + 40));
    *a3 = (int)(y - (new_y + 20));
    *loc_ptr = new_loc;

    return true;
}

// NOTE: Original code is likely different.
//
// 0x4B96F0
int64_t location_dist(int64_t a, int64_t b)
{
    int dx;
    int dy;

    dx = (int)LOCATION_GET_X(a) - (int)LOCATION_GET_X(b);
    dy = (int)LOCATION_GET_Y(a) - (int)LOCATION_GET_Y(b);

    if (dx < 0) {
        dx = -dx;
    }

    if (dy < 0) {
        dy = -dy;
    }

    if (dx > dy) {
        return dx;
    } else {
        return dy;
    }
}

// 0x4B9760
bool location_limits_set(int64_t x, int64_t y)
{
    if (x > 0x100000000 || y > 0x100000000) {
        return false;
    }

    location_origin_x = 0;
    location_origin_y = 0;
    location_limit_x = x;
    location_limit_y = y;
    qword_5FC2D8 = 20 * y;

    return true;
}

// 0x4B97E0
void location_limits_get(int64_t* x, int64_t* y)
{
    *x = location_limit_x;
    *y = location_limit_y;
}

// 0x4B9810
int64_t sub_4B9810()
{
    return location_make(location_limit_x / 2, location_limit_y / 2);
}

// 0x4B98B0
void sub_4B98B0(int64_t a1, int64_t a2, int64_t* a3, int64_t* a4)
{
    int v1;
    int v2;

    if (location_view_options.type == VIEW_TYPE_ISOMETRIC) {
        v1 = (int)((a1 - location_origin_x) / 2);
        v2 = (int)(a2 - location_origin_y);
        *a3 = (v2 - v1) / 40;
        *a4 = (v1 + v2) / 40;
    } else {
        *a3 = (location_origin_x + location_view_options.zoom - a1 - 1) / location_view_options.zoom;
        *a4 = (a2 - location_origin_y) / location_view_options.zoom;
    }
}

// 0x4B99C0
bool sub_4B99C0(int64_t from, int64_t* to)
{
    S603D20 v1;
    PathCreateInfo path_create_info;
    S603CB8 v2;
    S603CB8_F50 v3;
    int idx;

    sub_4F25E0(&v1);
    v1.aoe_flags = Tgt_Tile_Empty;
    v1.radius = 3;
    sub_4F2600(&v2, NULL, OBJ_HANDLE_NULL);

    v2.field_0 = &v1;
    v2.field_18 = from;
    v2.field_28 = from;
    v2.field_38 = from;
    if (sub_4F2D20(&v2)) {
        if (to != NULL) {
            *to = from;
        }
        return true;
    }

    v2.field_50 = &v3;
    v1.aoe_flags |= Tgt_Damaged_Poisoned;
    sub_4F40B0(&v2);

    for (idx = 0; idx < v3.cnt; idx++) {
        if (v3.entries[idx].loc != 0) {
            v2.field_28 = v3.entries[idx].loc;
            if (sub_4F2D20(&v2)) {
                path_create_info.obj = OBJ_HANDLE_NULL;
                path_create_info.from = from;
                path_create_info.to = v3.entries[idx].loc;
                path_create_info.max_rotations = sizeof(byte_5FC2A8);
                path_create_info.rotations = byte_5FC2A8;
                path_create_info.field_20 = 0x16;
                if (sub_41F3C0(&path_create_info)) {
                    if (to != NULL) {
                        *to = v3.entries[idx].loc;
                    }
                    return true;
                }
            }
        }
    }

    if (to != NULL) {
        *to = from;
    }
    return false;
}
