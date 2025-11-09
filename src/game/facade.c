#include "game/facade.h"

#include "game/tile.h"
#include "game/walkmask.h"

static void facade_load_data(int a1, int a2, int a3);
static void facade_free_data();
static void facade_get_data_screen_rect(TigRect* rect);

// 0x5FF570
static int facade_data_height;

// 0x5FF574
static int facade_data_width;

// 0x5FF578
static int64_t facade_data_origin_loc;

// 0x5FF580
static IsoInvalidateRectFunc* facade_iso_invalidate_rect;

// 0x5FF584
static bool facade_editor;

// 0x5FF588
static int facade_current_map_id;

// 0x5FF58C
static tig_window_handle_t facade_iso_window_handle;

// 0x5FF590
static bool facade_initialized;

// 0x5FF598
static ViewOptions facade_view_options;

// 0x5FF5A0
static tig_art_id_t* facade_art_id_grid;

// 0x5FF5A4
static TigVideoBuffer** facade_top_down_buffers;

// 0x4C9DA0
bool facade_init(GameInitInfo* init_info)
{
    facade_iso_window_handle = init_info->iso_window_handle;
    facade_iso_invalidate_rect = init_info->invalidate_rect_func;
    facade_editor = init_info->editor;
    facade_view_options.type = VIEW_TYPE_ISOMETRIC;
    facade_initialized = true;

    return true;
}

// 0x4C9DE0
void facade_exit()
{
    facade_free_data();
    facade_initialized = false;
}

// 0x4C9DF0
void facade_resize(GameResizeInfo* resize_info)
{
    facade_iso_window_handle = resize_info->window_handle;
}

// 0x4C9E00
void facade_update_view(ViewOptions* view_options)
{
    if (facade_art_id_grid != NULL) {
        facade_free_data();
        facade_view_options = *view_options;
        facade_load_data(facade_current_map_id, LOCATION_GET_X(facade_data_origin_loc), LOCATION_GET_Y(facade_data_origin_loc));
        facade_iso_invalidate_rect(NULL);
    } else {
        facade_view_options = *view_options;
    }
}

// 0x4C9E70
void facade_draw(GameDrawInfo* draw_info)
{
    LocRect loc_rect;
    int start_x;
    int start_y;
    TigRect tile_rect;
    int64_t y;
    int64_t x;
    TigArtBlitInfo art_blit_info;
    int index;
    int64_t tile_x;
    int64_t tile_y;
    TigRectListNode* rect_node;
    TigRect src_rect;
    TigRect dst_rect;

    if (facade_art_id_grid == NULL) {
        return;
    }

    loc_rect = *draw_info->loc_rect;
    if (!facade_clip_rect_and_get_start_indices(&loc_rect, &start_x, &start_y)) {
        return;
    }

    if (facade_view_options.type == VIEW_TYPE_TOP_DOWN) {
        tile_rect.width = facade_view_options.zoom;
        tile_rect.height = facade_view_options.zoom;
    } else {
        tile_rect.width = 80;
        tile_rect.height = 40;
    }

    art_blit_info.flags = 0;

    y = loc_rect.y1;
    while (y <= loc_rect.y2) {
        index = start_y * facade_data_width + start_x;

        x = loc_rect.x1;
        while (x <= loc_rect.x2) {
            art_blit_info.art_id = facade_art_id_grid[index];
            if (art_blit_info.art_id != TIG_ART_ID_INVALID) {
                location_xy(LOCATION_MAKE(x, y), &tile_x, &tile_y);
                tile_rect.x = (int)tile_x;
                tile_rect.y = (int)tile_y;

                if (facade_view_options.type == VIEW_TYPE_ISOMETRIC) {
                    tile_rect.x++;
                }

                rect_node = *draw_info->rects;
                while (rect_node != NULL) {
                    if (tig_rect_intersection(&tile_rect, &(rect_node->rect), &src_rect) == TIG_OK) {
                        dst_rect = src_rect;

                        src_rect.x -= (int)tile_x;
                        src_rect.y -= (int)tile_y;

                        art_blit_info.src_rect = &src_rect;
                        art_blit_info.dst_rect = &dst_rect;

                        if (facade_view_options.type == VIEW_TYPE_ISOMETRIC) {
                            src_rect.x--;
                            tig_window_blit_art(facade_iso_window_handle, &art_blit_info);
                        } else {
                            tig_window_copy_from_vbuffer(facade_iso_window_handle, &dst_rect, facade_top_down_buffers[index], &src_rect);
                        }
                    }
                    rect_node = rect_node->next;
                }
            }

            index++;
            x = LOCATION_MAKE(LOCATION_GET_X(x) + 1, LOCATION_GET_Y(x));
        }

        start_y++;
        y = LOCATION_MAKE(LOCATION_GET_X(y), LOCATION_GET_Y(y) + 1);
    }
}

// 0x4CA0F0
void facade_load_data(int a1, int a2, int a3)
{
    TigRect rect;
    TigVideoBufferCreateInfo vb_create_info;
    int index;

    facade_free_data();

    if (walkmask_load(a1, &facade_art_id_grid, &facade_data_width, &facade_data_height)) {
        facade_current_map_id = a1;
        facade_data_origin_loc = location_make(a2 - facade_data_width / 2, a3 - facade_data_height / 2);

        if (facade_view_options.type == VIEW_TYPE_TOP_DOWN) {
            facade_top_down_buffers = (TigVideoBuffer**)MALLOC(sizeof(*facade_top_down_buffers) * facade_data_height * facade_data_width);

            vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_SYSTEM_MEMORY;
            vb_create_info.width = facade_view_options.zoom;
            vb_create_info.height = facade_view_options.zoom;
            vb_create_info.background_color = 0;

            for (index = 0; index < facade_data_height * facade_data_width; index++) {
                if (facade_art_id_grid[index] != TIG_ART_ID_INVALID) {
                    tig_video_buffer_create(&vb_create_info, &(facade_top_down_buffers[index]));
                    sub_4D7590(facade_art_id_grid[index], facade_top_down_buffers[index]);
                } else {
                    facade_top_down_buffers[index] = NULL;
                }
            }
        }

        facade_get_data_screen_rect(&rect);
        facade_iso_invalidate_rect(&rect);
    }
}

// 0x4CA240
void facade_free_data()
{
    int index;

    if (facade_top_down_buffers != NULL) {
        for (index = 0; index < facade_data_width * facade_data_height; index++) {
            if (facade_top_down_buffers[index] != NULL) {
                tig_video_buffer_destroy(facade_top_down_buffers[index]);
            }
        }
        FREE(facade_top_down_buffers);
        facade_top_down_buffers = NULL;
    }

    if (facade_art_id_grid != NULL) {
        FREE(facade_art_id_grid);
        facade_art_id_grid = NULL;
    }
}

// 0x4CA2C0
void facade_set_or_update_palette()
{
    // TODO: Incomplete.
}

// 0x4CA6B0
bool facade_clip_rect_and_get_start_indices(LocRect* loc_rect, int* a2, int* a3)
{
    int64_t min_x;
    int64_t min_y;
    int64_t max_x;
    int64_t max_y;

    min_x = location_make(LOCATION_GET_X(facade_data_origin_loc), 0);
    min_y = location_make(0, LOCATION_GET_Y(facade_data_origin_loc));
    max_x = location_make(LOCATION_GET_X(facade_data_origin_loc) + facade_data_width - 1, 0);
    max_y = location_make(0, LOCATION_GET_Y(facade_data_origin_loc) + facade_data_height - 1);

    if (loc_rect->x1 < min_x) {
        loc_rect->x1 = min_x;
    }

    if (loc_rect->y1 < min_y) {
        loc_rect->y1 = min_y;
    }

    if (loc_rect->x2 >= max_x) {
        loc_rect->x2 = max_x;
    }

    if (loc_rect->y2 >= max_y) {
        loc_rect->y2 = max_y;
    }

    if (facade_data_origin_loc > loc_rect->y1) {
        loc_rect->y1 = facade_data_origin_loc;
    }

    if (loc_rect->x2 - loc_rect->x1 + 1 < 0) {
        return false;
    }

    if (loc_rect->y2 - loc_rect->y1 + 1 < 0) {
        return false;
    }

    *a2 = (int)(loc_rect->x1 - min_x);
    *a3 = (int)(loc_rect->y1 - min_y);

    return true;
}

// 0x4CA7C0
void facade_get_data_screen_rect(TigRect* rect)
{
    int64_t tmp;
    int64_t min_x;
    int64_t min_y;
    int64_t max_x;
    int64_t max_y;

    switch (facade_view_options.type) {
    case VIEW_TYPE_ISOMETRIC:
        location_xy(facade_data_origin_loc, &tmp, &min_y);
        location_xy(LOCATION_MAKE(LOCATION_GET_X(facade_data_origin_loc) + facade_data_width, LOCATION_GET_Y(facade_data_origin_loc)), &min_x, &tmp);
        location_xy(LOCATION_MAKE(LOCATION_GET_X(facade_data_origin_loc), LOCATION_GET_Y(facade_data_origin_loc) + facade_data_width), &max_x, &tmp);
        location_xy(LOCATION_MAKE(LOCATION_GET_X(facade_data_origin_loc) + facade_data_width, LOCATION_GET_Y(facade_data_origin_loc) + facade_data_width), &tmp, &max_y);
        max_x += 80;
        max_y += 40;
        break;
    case VIEW_TYPE_TOP_DOWN:
        location_xy(facade_data_origin_loc, &min_x, &min_y);
        location_xy(LOCATION_MAKE(LOCATION_GET_X(facade_data_origin_loc) + facade_data_width, LOCATION_GET_Y(facade_data_origin_loc) + facade_data_width), &max_x, &max_y);
        max_x += facade_view_options.zoom;
        max_y += facade_view_options.zoom;
        break;
    default:
        // Should be unreachable.
        assert(0);
    }

    rect->x = (int)min_x;
    rect->y = (int)min_y;
    rect->width = (int)(max_x - min_x + 1);
    rect->height = (int)(max_y - min_y + 1);
}
