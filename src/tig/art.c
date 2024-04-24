#include "tig/art.h"

#include <stdio.h>

#include "tig/bmp.h"
#include "tig/color.h"
#include "tig/core.h"
#include "tig/debug.h"
#include "tig/file.h"
#include "tig/memory.h"
#include "tig/palette.h"
#include "tig/timer.h"
#include "tig/video.h"

#define ART_ID_TYPE_SHIFT 28
#define ART_ID_PALETTE_SHIFT 4
#define ART_ID_ROTATION_SHIFT 11

#define LIGHT_ID_ROTATION_SHIFT 9

#define EYE_CANDY_ID_ROTATION_SHIFT 9

#define MAX_PALETTES 4
#define MAX_ROTATIONS 8

// Represents memory size used in art cache.
//
// The type is signed. When used in "available" memory variables the value below
// zero denotes we've used too much memory and should evict something.
typedef int art_size_t;

typedef struct TigArtFileFrameData {
    /* 0000 */ int width;
    /* 0004 */ int height;
    /* 0008 */ int data_size;
    /* 000C */ int hot_x;
    /* 0010 */ int hot_y;
    /* 0014 */ int offset_x;
    /* 0018 */ int offset_y;
} TigArtFileFrameData;

static_assert(sizeof(TigArtFileFrameData) == 0x1C, "wrong size");

typedef struct TigArtHeader {
    /* 0000 */ unsigned int flags;
    /* 0004 */ int fps;
    /* 0008 */ int bpp;
    /* 000C */ uint32_t* palette_tbl[MAX_PALETTES];
    /* 001C */ int action_frame;
    /* 0020 */ int num_frames;
    /* 0024 */ TigArtFileFrameData* frames_tbl[MAX_ROTATIONS];
    /* 0044 */ int data_size[MAX_ROTATIONS];
    /* 0064 */ uint8_t* pixels_tbl[MAX_ROTATIONS];
} TigArtHeader;

static_assert(sizeof(TigArtHeader) == 0x84, "wrong size");

typedef struct TigArtFrameSave {
    /* 0000 */ int field_0;
    /* 0004 */ int field_4;
    /* 0008 */ int field_8;
    /* 000C */ int field_C;
} TigArtFrameSave;

static_assert(sizeof(TigArtFrameSave) == 0x10, "wrong size");

typedef struct TigShdHeader {
    /* 0000 */ unsigned int flags;
    /* 0004 */ int num_frames;
    /* 0004 */ TigArtFrameSave* field_8[MAX_ROTATIONS];
    /* 0028 */ int field_28[MAX_ROTATIONS];
    /* 0048 */ void* field_48[MAX_ROTATIONS];
} TigShdHeader;

static_assert(sizeof(TigShdHeader) == 0x68, "wrong size");

#define TIG_ART_CACHE_ENTRY_MODIFIED 0x02

typedef struct TigArtCacheEntry {
    /* 0000 */ unsigned int flags;
    /* 0004 */ char path[MAX_PATH];
    /* 0108 */ timer_t time;
    /* 010C */ unsigned int art_id;
    /* 0110 */ TigArtHeader hdr;
    /* 0194 */ uint8_t field_194[MAX_PALETTES][MAX_ROTATIONS];
    /* 01B4 */ TigVideoBuffer** video_buffers[MAX_PALETTES][MAX_ROTATIONS];
    /* 0234 */ uint8_t** pixels_tbl[MAX_ROTATIONS];
    /* 0254 */ TigPalette field_254[MAX_PALETTES];
    /* 0264 */ art_size_t system_memory_usage;
    /* 0268 */ art_size_t video_memory_usage;
} TigArtCacheEntry;

static_assert(sizeof(TigArtCacheEntry) == 0x26C, "wrong size");

static int art_get_video_buffer(unsigned int cache_entry_index, tig_art_id_t art_id, TigVideoBuffer** video_buffer_ptr);
static int sub_505940(unsigned int art_blt_flags, unsigned int* vb_blt_flags_ptr);
static int sub_5059F0(int cache_entry_index, TigArtBlitSpec* blt);
static int art_blit(int cache_entry_index, TigArtBlitSpec* blt);
static unsigned int sub_51AA90(tig_art_id_t art_id);
static void tig_art_cache_check_fullness();
static int tig_art_cache_entry_compare_time(const void* a1, const void* a2);
static int tig_art_cache_entry_compare_name(const void* a1, const void* a2);
static int tig_art_build_path(unsigned int art_id, char* path);
static bool tig_art_cache_find(const char* path, int* index);
static bool tig_art_cache_entry_load(tig_art_id_t art_id, const char* path, int index);
static void tig_art_cache_entry_unload(unsigned int cache_entry_index);
static void sub_51B610(unsigned int cache_entry_index);
static void sub_51B650(int cache_entry_index);
static int sub_51B710(tig_art_id_t art_id, const char* filename, TigArtHeader* hdr, void** palettes, int a5, art_size_t* size_ptr);
static int sub_51BE30(TigArtHeader* hdr);
static void sub_51BE50(TigFile* stream, TigArtHeader* hdr, TigPalette* palette_tbl);
static void sub_51BEB0(FILE* stream, TigArtHeader* hdr, const char* filename, FILE* shd_stream, TigShdHeader* shd, const char* shd_filename);
static void sub_51BF20(TigArtHeader* hdr);
static void sub_51BF60(TigShdHeader* shd);
static int sub_51BFB0(FILE* stream, uint8_t* pixels, int width, int height, int pitch, TigArtHeader* hdr, int rotation, int frame);
static int sub_51C080(FILE* stream, uint8_t* pixels, int width, int height, int pitch);
static int sub_51C3C0(FILE* stream, uint8_t* pixels, int width, int height, int pitch, int* a6);
static int sub_51C4E0(int a1, TigBmp* bmp, TigRect* content_rect, int* pitch_ptr, int* a5, int* a6, int* a7, bool a8, bool a9);
static void sub_51C6D0(uint8_t* pixels, const TigRect* rect, int pitch, TigRect* content_rect);
static int sub_51C890(int frame, TigBmp* bmp, TigRect* content_rect, int* pitch_ptr, int* width_ptr, int* height_ptr);
static int sub_51CA80(uint8_t* pixels, int pitch, int height, int start);

// 0x5BE880
static int dword_5BE880[16] = {
    0,
    1,
    8,
    3,
    4,
    5,
    6,
    7,
    8,
    3,
    10,
    11,
    6,
    7,
    14,
    15,
};

// 0x5BE8C0
static int dword_5BE8C0[16] = {
    0,
    1,
    2,
    9,
    4,
    5,
    12,
    13,
    2,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
};

// 0x5BE900
static int dword_5BE900[32] = {
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    1,
    1,
    1,
    0,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
};

// 0x5BE980
static int dword_5BE980[5] = {
    1,
    0,
    0,
    2,
    1,
};

// 0x5BE994
static int dword_5BE994[32] = {
    0,
    0,
    1,
    2,
    1,
    1,
    1,
    1,
    2,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    0,
    2,
    2,
    2,
    1,
    1,
    1,
};

// 0x5BEA14
static bool tig_art_mirroring_enabled = true;

// 0x5BEA18
static int dword_5BEA18[4] = {
    6,
    1,
    3,
    2,
};

// 0x5BEA28
static int dword_5BEA28[4] = {
    2,
    4,
    1,
    5,
};

// 0x5BEA38
float tig_art_cache_video_memory_fullness = 0.3f;

// 0x604710
static TigArtFilePathResolver* tig_art_file_path_resolver;

// 0x604714
static unsigned int dword_604714;

// 0x604718
static bool dword_604718;

// 0x60471C
static unsigned int tig_art_cache_entries_length;

// 0x604720
static TigArtCacheEntry* tig_art_cache_entries;

// 0x604724
static art_size_t tig_art_total_system_memory;

// 0x60472C
static unsigned int tig_art_cache_entries_capacity;

// 0x604730
static art_size_t tig_art_available_video_memory;

// 0x604744
static TigArtBlitPaletteAdjustCallback* dword_604744;

// 0x604738
static int tig_art_bytes_per_pixel;

// 0x60473C
static bool tig_art_initialized;

// 0x604740
static art_size_t tig_art_available_system_memory;

// 0x604748
static art_size_t tig_art_total_video_memory;

// 0x60474C
static int tig_art_bits_per_pixel;

// 0x604750
static TigArtNormalizeIdHandler* dword_604750;

// 0x604754
static int dword_604754;

// 0x500590
int tig_art_init(TigInitializeInfo* init_info)
{
    size_t total_memory;
    size_t available_memory;

    if (tig_art_initialized) {
        return TIG_ALREADY_INITIALIZED;
    }

    tig_art_cache_entries_capacity = 512;
    tig_art_cache_entries_length = 0;
    tig_art_cache_entries = (TigArtCacheEntry*)MALLOC(sizeof(TigArtCacheEntry) * tig_art_cache_entries_capacity);

    tig_memory_get_system_status(&total_memory, &available_memory);
    tig_art_available_system_memory = (art_size_t)(total_memory >> 2);
    tig_art_total_system_memory = (art_size_t)(total_memory >> 2);

    if (tig_video_get_video_memory_status(&total_memory, &available_memory) != TIG_OK) {
        available_memory = tig_art_total_system_memory;
    }

    tig_art_available_video_memory = (art_size_t)(available_memory / 2);
    tig_art_total_video_memory = tig_art_available_video_memory;
    tig_debug_printf("Art mem vid avail is %d\n", tig_art_available_video_memory);

    tig_art_bits_per_pixel = init_info->bpp;
    tig_art_bytes_per_pixel = tig_art_bits_per_pixel / 8;
    tig_art_file_path_resolver = init_info->art_file_path_resolver;
    dword_604750 = init_info->art_normalize_id_handler;

    tig_art_initialized = true;

    dword_604718 = tig_video_3d_check_initialized() == TIG_OK;

    return TIG_OK;
}

// 0x500690
void tig_art_exit()
{
    if (tig_art_initialized) {
        tig_art_flush();

        if (tig_art_cache_entries != NULL) {
            FREE(tig_art_cache_entries);
            tig_art_cache_entries = NULL;
            tig_art_cache_entries_length = 0;
            tig_art_cache_entries_capacity = 0;
        }

        tig_art_initialized = false;
    }
}

// 0x5006D0
void tig_art_ping()
{
}

// 0x5006E0
int sub_5006E0(tig_art_id_t art_id, TigPalette palette)
{
    TigArtHeader hdr;
    art_size_t size;
    TigPalette palette_tbl[MAX_PALETTES];
    char path[_MAX_PATH];
    int rc;

    rc = tig_art_build_path(art_id, path);
    if (rc != TIG_OK) {
        return rc;
    }

    palette_tbl[0] = palette;
    return sub_51B710(art_id,
        path,
        &hdr,
        palette_tbl,
        1,
        &size);
}

// 0x501DD0
int tig_art_misc_id_create(unsigned int a1, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (a1 >= 512
        || palette >= MAX_PALETTES) {
        return TIG_ERR_12;
    }

    *art_id_ptr = (TIG_ART_TYPE_MISC << ART_ID_TYPE_SHIFT)
        | ((a1 & 0x1FF) << 19)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT);

    return TIG_OK;
}

// 0x501E10
int tig_art_set_fps(tig_art_id_t art_id, int fps)
{
    int index;

    index = sub_51AA90(art_id);
    if (index == -1) {
        return TIG_ERR_16;
    }

    tig_art_cache_entries[index].hdr.fps = fps;
    tig_art_cache_entries[index].flags |= TIG_ART_CACHE_ENTRY_MODIFIED;

    return TIG_OK;
}

// 0x501E60
int tig_art_set_action_frame(tig_art_id_t art_id, short action_frame)
{
    int index;

    index = sub_51AA90(art_id);
    if (index == -1) {
        return TIG_ERR_16;
    }

    tig_art_cache_entries[index].hdr.action_frame = action_frame;
    tig_art_cache_entries[index].flags |= TIG_ART_CACHE_ENTRY_MODIFIED;

    return TIG_OK;
}

// 0x501EB0
int sub_501EB0(tig_art_id_t art_id, const char* filename)
{
    int index;
    FILE* stream;

    index = sub_51AA90(art_id);
    if (index == -1) {
        return TIG_ERR_16;
    }

    if ((tig_art_cache_entries[index].flags & TIG_ART_CACHE_ENTRY_MODIFIED) == 0) {
        return TIG_ERR_16;
    }

    tig_art_cache_entries[index].flags &= ~TIG_ART_CACHE_ENTRY_MODIFIED;

    stream = fopen(filename, "r+b");
    if (stream == NULL) {
        return TIG_ERR_13;
    }

    rewind(stream);

    if (fwrite(&(tig_art_cache_entries[index].hdr), sizeof(TigArtHeader), 1, stream) != 1) {
        fclose(stream);
        return TIG_ERR_13;
    }

    fclose(stream);
    return TIG_OK;
}

// 0x501F60
int sub_501F60(const char* filename, uint32_t* new_palette_entries, int new_palette_index)
{
    FILE* in;
    FILE* out;
    TigArtHeader hdr;
    char path[_MAX_PATH];
    uint32_t palette_entries[256];
    uint8_t buffer[10000];
    uint32_t* palette_table[MAX_PALETTES];
    int index;
    size_t bytes_read;
    size_t bytes_written;

    if (tmpnam(path) == NULL) {
        return TIG_ERR_13;
    }

    in = fopen(filename, "rb");
    if (in == NULL) {
        return TIG_ERR_13;
    }

    out = fopen(path, "wb");
    if (out == NULL) {
        fclose(in);
        return TIG_ERR_13;
    }

    if (fread(&hdr, sizeof(hdr), 1, in) != 1) {
        fclose(in);
        fclose(out);
        return TIG_ERR_13;
    }

    palette_table[0] = hdr.palette_tbl[0];
    palette_table[1] = hdr.palette_tbl[1];
    palette_table[2] = hdr.palette_tbl[2];
    palette_table[3] = hdr.palette_tbl[3];
    hdr.palette_tbl[new_palette_index] = new_palette_entries;

    if (fwrite(&hdr, sizeof(hdr), 1, out) != 1) {
        fclose(in);
        fclose(out);
        return TIG_ERR_13;
    }

    for (index = 0; index < MAX_PALETTES; index++) {
        if (palette_table[index] != 0) {
            if (fread(palette_entries, sizeof(*palette_entries), 256, in) != 256) {
                fclose(in);
                fclose(out);
                return TIG_ERR_13;
            }
        }

        if (new_palette_index == index) {
            if (fwrite(new_palette_entries, sizeof(*new_palette_entries), 256, out) != 256) {
                fclose(in);
                fclose(out);
                return TIG_ERR_13;
            }
        }

        if (palette_table[index] != 0) {
            if (fwrite(palette_entries, sizeof(*palette_entries), 256, out) != 256) {
                fclose(in);
                fclose(out);
                return TIG_ERR_13;
            }
        }
    }

    for (;;) {
        bytes_read = fread(buffer, 1, sizeof(buffer), in);
        if (bytes_read == 0) {
            break;
        }

        bytes_written = fwrite(buffer, 1, bytes_read, out);
        if (bytes_read != bytes_written) {
            fclose(in);
            fclose(out);
            return TIG_ERR_13;
        }
    }

    fclose(in);
    fclose(out);
    if (!CopyFileA(path, filename, FALSE)) {
        return TIG_ERR_16;
    }

    remove(path);
    return TIG_OK;
}

// 0x5021E0
void tig_art_flush()
{
    unsigned int index;

    if (!tig_art_initialized) {
        return;
    }

    for (index = 0; index < tig_art_cache_entries_length; index++) {
        tig_art_cache_entry_unload(index);
    }

    tig_art_cache_entries_length = 0;
}

// 0x502220
int tig_art_exists(tig_art_id_t art_id)
{
    char path[MAX_PATH];
    if (tig_art_build_path(art_id, path) != TIG_OK) {
        return TIG_ERR_16;
    }

    if (!tig_file_exists(path, NULL)) {
        return TIG_ERR_16;
    }

    return TIG_OK;
}

// 0x502270
int sub_502270(tig_art_id_t art_id, char* path)
{
    return tig_art_build_path(art_id, path);
}

// 0x502290
int sub_502290(tig_art_id_t art_id)
{
    int index;

    index = sub_51AA90(art_id);
    if (index == -1) {
        return TIG_ERR_13;
    }

    return TIG_OK;
}

// 0x5022B0
void sub_5022B0(TigArtBlitPaletteAdjustCallback* callback)
{
    dword_604744 = callback;
}

// 0x5022C0
TigArtBlitPaletteAdjustCallback* sub_5022C0()
{
    return dword_604744;
}

// 0x5022D0
void sub_5022D0()
{
    unsigned int cache_entry_index;
    unsigned int palette;

    for (cache_entry_index = 0; cache_entry_index < tig_art_cache_entries_length; cache_entry_index++) {
        for (palette = 0; palette < MAX_PALETTES; palette++) {
            if (tig_art_cache_entries[cache_entry_index].hdr.palette_tbl[palette] != NULL) {
                sub_505000(tig_art_cache_entries[cache_entry_index].art_id,
                    tig_art_cache_entries[cache_entry_index].hdr.palette_tbl[palette],
                    tig_art_cache_entries[cache_entry_index].field_254[palette]);
            }
        }
        sub_51B610(cache_entry_index);
    }
}

// 0x502360
int tig_art_blit(TigArtBlitSpec* blit_spec)
{
    TigArtBlitSpec mut_art_blt;
    TigVideoBuffer* video_buffer;
    TigVideoBufferData video_buffer_data;
    TigVideoBufferBlitInfo vb_blit_info;
    int rc;
    unsigned int cache_entry_index;
    unsigned int type;

    mut_art_blt = *blit_spec;

    cache_entry_index = sub_51AA90(mut_art_blt.art_id);
    if (cache_entry_index == -1) {
        return TIG_ERR_13;
    }

    type = tig_art_type(mut_art_blt.art_id);
    if (type == TIG_ART_TYPE_TILE) {
        if (sub_503900(mut_art_blt.art_id)) {
            unsigned int v1 = sub_504FD0(mut_art_blt.art_id);
            if ((v1 & 0x1) != 0) {
                if ((mut_art_blt.flags & TIG_ART_BLT_MIRROR_LEFT_RIGHT) != 0) {
                    mut_art_blt.flags &= ~TIG_ART_BLT_MIRROR_LEFT_RIGHT;
                } else {
                    mut_art_blt.flags |= TIG_ART_BLT_MIRROR_LEFT_RIGHT;
                }
            }
            mut_art_blt.art_id = sub_502D30(mut_art_blt.art_id, v1 & ~0x1);
        }
    } else {
        if (tig_art_mirroring_enabled
            && (type == TIG_ART_TYPE_CRITTER
                || type == TIG_ART_TYPE_MONSTER
                || type == TIG_ART_TYPE_UNIQUE_NPC)) {
            int rotation = tig_art_id_rotation_get(mut_art_blt.art_id);
            if (rotation > 0 && rotation < 4) {
                mut_art_blt.art_id = tig_art_id_rotation_set(mut_art_blt.art_id, MAX_ROTATIONS - rotation);
                if ((mut_art_blt.flags & TIG_ART_BLT_MIRROR_LEFT_RIGHT) != 0) {
                    mut_art_blt.flags &= ~TIG_ART_BLT_MIRROR_LEFT_RIGHT;
                } else {
                    mut_art_blt.flags |= TIG_ART_BLT_MIRROR_LEFT_RIGHT;
                }
            }
        }
    }

    if (sub_505940(mut_art_blt.flags, &(vb_blit_info.flags)) == TIG_OK
        && sub_520FB0(mut_art_blt.dst_video_buffer, vb_blit_info.flags) == TIG_OK
        && art_get_video_buffer(cache_entry_index, mut_art_blt.art_id, &video_buffer) == TIG_OK) {
        if ((mut_art_blt.flags & TIG_ART_BLT_0x2000) != 0) {
            vb_blit_info.field_10 = mut_art_blt.field_10;
        } else if ((mut_art_blt.flags & TIG_ART_BLT_0x20000) != 0) {
            vb_blit_info.field_10 = mut_art_blt.field_14[0];
            vb_blit_info.field_14 = mut_art_blt.field_14[1];
            vb_blit_info.field_18 = mut_art_blt.field_14[2];
            vb_blit_info.field_1C = mut_art_blt.field_14[3];
            vb_blit_info.field_20 = mut_art_blt.field_18;
        } else if ((mut_art_blt.flags & TIG_ART_BLT_0x4000) != 0) {
            vb_blit_info.field_10 = mut_art_blt.field_14[0];
            vb_blit_info.field_14 = mut_art_blt.field_14[1];
            vb_blit_info.field_18 = mut_art_blt.field_14[1];
            vb_blit_info.field_1C = mut_art_blt.field_14[0];
            vb_blit_info.field_20 = 0;
        }

        if ((mut_art_blt.flags & TIG_ART_BLT_BLEND_MODE_0x100) != 0) {
            vb_blit_info.field_C = mut_art_blt.opacity[0];
        } else if ((mut_art_blt.flags & TIG_ART_BLT_BLEND_MODE_GRADIENT_HORIZONTAL) != 0) {
            vb_blit_info.field_C = mut_art_blt.opacity[0];
            vb_blit_info.field_D = mut_art_blt.opacity[0];
            vb_blit_info.field_E = mut_art_blt.opacity[1];
            vb_blit_info.field_F = mut_art_blt.opacity[1];
        } else if ((mut_art_blt.flags & TIG_ART_BLT_BLEND_MODE_GRADIENT_VERTICAL) != 0) {
            vb_blit_info.field_C = mut_art_blt.opacity[0];
            vb_blit_info.field_D = mut_art_blt.opacity[0];
            vb_blit_info.field_E = mut_art_blt.opacity[3];
            vb_blit_info.field_F = mut_art_blt.opacity[3];
        } else if ((mut_art_blt.flags & TIG_ART_BLT_BLEND_MODE_GRADIENT_CORNERS) != 0) {
            vb_blit_info.field_C = mut_art_blt.opacity[0];
            vb_blit_info.field_D = mut_art_blt.opacity[1];
            vb_blit_info.field_E = mut_art_blt.opacity[2];
            vb_blit_info.field_F = mut_art_blt.opacity[3];
        }

        vb_blit_info.src_rect = mut_art_blt.src_rect;
        vb_blit_info.src_video_buffer = video_buffer;
        vb_blit_info.dst_rect = mut_art_blt.dst_rect;
        vb_blit_info.dst_video_buffer = mut_art_blt.dst_video_buffer;
        return tig_video_buffer_blit(&vb_blit_info);
    }

    if ((mut_art_blt.flags & TIG_ART_BLT_0x0003FFF0) != 0) {
        tig_video_buffer_data(mut_art_blt.dst_video_buffer, &video_buffer_data);

        if ((video_buffer_data.flags & TIG_VIDEO_BUFFER_VIDEO_MEMORY) != 0
            && (mut_art_blt.flags & TIG_ART_BLT_0x01000000) != 0) {
            rc = tig_video_buffer_data(mut_art_blt.scratch_video_buffer, &video_buffer_data);
            if (rc != TIG_OK) {
                return rc;
            }

            if ((mut_art_blt.flags & TIG_ART_BLT_0x00019FF0) != 0) {
                if ((video_buffer_data.flags & TIG_VIDEO_BUFFER_SYSTEM_MEMORY) == 0) {
                    tig_debug_printf("Warning: using video memory surface as a scratch buffer.  This will cause slower performance.\n");
                }

                vb_blit_info.flags = 0;
                vb_blit_info.src_rect = mut_art_blt.dst_rect;
                vb_blit_info.dst_rect = mut_art_blt.dst_rect;
                vb_blit_info.src_video_buffer = mut_art_blt.dst_video_buffer;
                vb_blit_info.dst_video_buffer = mut_art_blt.scratch_video_buffer;
                rc = tig_video_buffer_blit(&vb_blit_info);
                if (rc != TIG_OK) {
                    return rc;
                }
            } else {
                tig_video_buffer_fill(mut_art_blt.scratch_video_buffer,
                    mut_art_blt.dst_rect,
                    video_buffer_data.color_key);
            }

            video_buffer = mut_art_blt.dst_video_buffer;
            mut_art_blt.dst_video_buffer = mut_art_blt.scratch_video_buffer;

            rc = art_blit(cache_entry_index, &mut_art_blt);
            if (rc != TIG_OK) {
                return rc;
            }

            mut_art_blt.dst_video_buffer = video_buffer;

            vb_blit_info.flags = 0;
            vb_blit_info.src_rect = mut_art_blt.dst_rect;
            vb_blit_info.src_video_buffer = mut_art_blt.scratch_video_buffer;
            vb_blit_info.dst_rect = mut_art_blt.dst_rect;
            vb_blit_info.dst_video_buffer = mut_art_blt.dst_video_buffer;
            return tig_video_buffer_blit(&vb_blit_info);
        }
    }

    return art_blit(cache_entry_index, &mut_art_blt);
}

// 0x502700
int tig_art_type(tig_art_id_t art_id)
{
    return art_id >> ART_ID_TYPE_SHIFT;
}

// 0x502710
int tig_art_num(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
        return 0;
    case TIG_ART_TYPE_UNIQUE_NPC:
        return (art_id >> 20) & 0xFF;
    case TIG_ART_TYPE_ITEM:
        return (art_id >> 17) & 0x7FF;
    case TIG_ART_TYPE_INTERFACE:
        return (art_id >> 16) & 0xFFF;
    default:
        return (art_id >> 19) & 0x1FF;
    }
}

// 0x502780
tig_art_id_t sub_502780(tig_art_id_t art_id, unsigned int value)
{
    unsigned int max;
    unsigned int mask;
    unsigned int shift;

    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
        return 0;
    case TIG_ART_TYPE_UNIQUE_NPC:
        max = 256;
        mask = 0xFF00000;
        shift = 20;
        break;
    case TIG_ART_TYPE_ITEM:
        max = sub_502830(art_id);
        mask = 0xFFE0000;
        shift = 17;
        break;
    case TIG_ART_TYPE_INTERFACE:
        max = 4096;
        mask = 0xFFF0000;
        shift = 16;
        break;
    default:
        max = 512;
        mask = 0xFF80000;
        shift = 19;
        break;
    }

    if (value < max) {
        return (art_id & ~mask) | (value << shift);
    }

    tig_debug_println("Range exceeded in art set.");

    return art_id;
}

// 0x502830
unsigned int sub_502830(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ITEM) {
        return 0;
    }

    unsigned int max = 1000;

    switch (sub_504550(art_id)) {
    case TIG_ART_TYPE_6_SUBTYPE_0:
    case TIG_ART_TYPE_6_SUBTYPE_1:
        max = 20;
        break;
    case TIG_ART_TYPE_6_SUBTYPE_2:
        if (sub_504570(art_id) == 0) {
            max = 20;
        }
        break;
    }

    return max;
}

// 0x502880
int tig_art_id_rotation_get(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_INTERFACE:
    case TIG_ART_TYPE_MISC:
    case TIG_ART_TYPE_ROOF:
    case TIG_ART_TYPE_ITEM:
    case TIG_ART_TYPE_FACADE:
        return 0;
    case TIG_ART_TYPE_LIGHT:
        return (art_id >> LIGHT_ID_ROTATION_SHIFT) & (MAX_ROTATIONS - 1);
    case TIG_ART_TYPE_EYE_CANDY:
        return (art_id >> EYE_CANDY_ID_ROTATION_SHIFT) & (MAX_ROTATIONS - 1);
    default:
        return (art_id >> ART_ID_ROTATION_SHIFT) & (MAX_ROTATIONS - 1);
    }
}

// 0x5028E0
tig_art_id_t tig_art_id_rotation_inc(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_INTERFACE:
    case TIG_ART_TYPE_MISC:
    case TIG_ART_TYPE_ROOF:
    case TIG_ART_TYPE_ITEM:
    case TIG_ART_TYPE_FACADE:
        return art_id;
    }

    return tig_art_id_rotation_set(art_id, tig_art_id_rotation_get(art_id) + 1);
}

// 0x502930
tig_art_id_t tig_art_id_rotation_set(tig_art_id_t art_id, int rotation)
{
    int type;
    unsigned int mask;
    int shift;
    int old_rotation;
    int old_cardinal_direction;
    int cardinal_direction;
    int v1;
    int v2;
    int v3;

    type = tig_art_type(art_id);

    switch (type) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_INTERFACE:
    case TIG_ART_TYPE_ITEM:
    case TIG_ART_TYPE_MISC:
    case TIG_ART_TYPE_ROOF:
    case TIG_ART_TYPE_FACADE:
        return art_id;
    }

    if (!sub_504790(art_id)) {
        return art_id;
    }

    switch (type) {
    case TIG_ART_TYPE_EYE_CANDY:
        mask = 0xE00;
        shift = EYE_CANDY_ID_ROTATION_SHIFT;
        break;
    case TIG_ART_TYPE_LIGHT:
        mask = 0xE00;
        shift = LIGHT_ID_ROTATION_SHIFT;
        break;
    default:
        mask = 0x3800;
        shift = ART_ID_ROTATION_SHIFT;
        break;
    }

    if (rotation >= MAX_ROTATIONS) {
        rotation = 0;
    } else if (rotation < 0) {
        rotation = MAX_ROTATIONS - 1;
    }

    old_rotation = tig_art_id_rotation_get(art_id);
    art_id = (art_id & ~mask) | (rotation << shift);

    if (type == TIG_ART_TYPE_WALL) {
        v1 = sub_503A60(art_id);
        old_cardinal_direction = old_rotation / 2;
        cardinal_direction = rotation / 2;

        if (old_cardinal_direction != cardinal_direction) {
            if (dword_5BEA18[old_cardinal_direction] == v1) {
                v1 = dword_5BEA18[cardinal_direction];
            } else if (dword_5BEA28[old_cardinal_direction] == v1) {
                v1 = dword_5BEA28[cardinal_direction];
            } else if (dword_5BEA18[(old_cardinal_direction + 2) % 4] == v1) {
                v1 = dword_5BEA18[(cardinal_direction + 2) % 4];
            } else if (dword_5BEA28[(old_cardinal_direction + 2) % 4] == v1) {
                v1 = dword_5BEA28[(cardinal_direction + 2) % 4];
            }

            art_id = sub_503A90(art_id, v1);
            if ((old_cardinal_direction == 0 || old_cardinal_direction == 3)
                    && (cardinal_direction == 1 || cardinal_direction == 2)
                || (old_cardinal_direction == 1 || old_cardinal_direction == 2)
                    && (cardinal_direction == 0 || cardinal_direction == 3)) {
                v2 = sub_503B70(art_id);
                v3 = 0;
                if ((v2 & 0x400) != 0) {
                    v3 |= 0x80;
                }
                if ((v2 & 0x80) != 0) {
                    v3 |= 0x400;
                }
                art_id = sub_503BB0(art_id, v3);
            }
        }

        if (cardinal_direction != 1 && cardinal_direction != 3) {
            art_id = sub_502D30(art_id, 0);
            art_id = tig_art_set_palette(art_id, 0);
        } else {
            art_id = sub_502D30(art_id, 1);
            art_id = tig_art_set_palette(art_id, 1);
        }
    } else if (type == TIG_ART_TYPE_PORTAL) {
        if (rotation / 2 != 1 && rotation / 2 != 3) {
            art_id = sub_502D30(art_id, 0);
            art_id = tig_art_set_palette(art_id, 0);
        } else {
            art_id = sub_502D30(art_id, 1);
            art_id = tig_art_set_palette(art_id, 1);
        }
    }

    return art_id;
}

// 0x502B50
int tig_art_id_frame_get(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_ITEM:
        return 0;
    case TIG_ART_TYPE_INTERFACE:
    case TIG_ART_TYPE_MISC:
        return (art_id >> 8) & 0xFF;
    case TIG_ART_TYPE_FACADE:
        return (art_id >> 1) & 0x3FF;
    case TIG_ART_TYPE_LIGHT:
        return (art_id >> 12) & 0x7F;
    case TIG_ART_TYPE_EYE_CANDY:
        return (art_id >> 12) & 0x7F;
    default:
        return (art_id >> 14) & 0x1F;
    }
}

// 0x502BC0
tig_art_id_t tig_art_id_frame_inc(tig_art_id_t art_id)
{
    int frame;

    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_ITEM:
        return art_id;
    }

    frame = tig_art_id_frame_get(art_id);
    return tig_art_id_frame_set(art_id, frame + 1);
}

// 0x502C00
tig_art_id_t tig_art_id_frame_dec(tig_art_id_t art_id)
{
    int frame;

    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_ITEM:
        return art_id;
    }

    frame = tig_art_id_frame_get(art_id);
    if (frame > 0) {
        --frame;
    }

    return tig_art_id_frame_set(art_id, frame);
}

// 0x502C40
tig_art_id_t tig_art_id_frame_set(tig_art_id_t art_id, int value)
{
    TigArtData art_data;
    int type;

    type = tig_art_type(art_id);

    if (type == TIG_ART_TYPE_TILE
        || type == TIG_ART_TYPE_WALL
        || type == TIG_ART_TYPE_ITEM) {
        return art_id;
    }

    if (value > 0) {
        if (tig_art_data(art_id, &art_data) != TIG_OK) {
            return art_id;
        }

        if (value >= art_data.num_frames) {
            value = 0;
        }
    } else if (value < 0) {
        if (tig_art_data(art_id, &art_data) != TIG_OK) {
            return art_id;
        }

        value = art_data.num_frames - 1;
    }

    switch (type) {
    case TIG_ART_TYPE_INTERFACE:
    case TIG_ART_TYPE_MISC:
        return (art_id & ~0xFF00) | (value << 8);
    case TIG_ART_TYPE_FACADE:
        return (art_id & ~0x7FE) | (value << 1);
    case TIG_ART_TYPE_LIGHT:
    case TIG_ART_TYPE_EYE_CANDY:
        return (art_id & ~0x7F000) | (value << 12);
    }

    return (art_id & ~0x7C000) | (value << 14);
}

// 0x502D30
tig_art_id_t sub_502D30(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_PORTAL:
    case TIG_ART_TYPE_ROOF:
        if (value >= 16) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0xF) | value;
    default:
        return art_id;
    }
}

// 0x502D80
int tig_art_palette(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_LIGHT:
    case TIG_ART_TYPE_FACADE:
        return 0;
    default:
        return (art_id >> ART_ID_PALETTE_SHIFT) & (MAX_PALETTES - 1);
    }
}

// 0x502DB0
tig_art_id_t tig_art_set_palette(tig_art_id_t art_id, int value)
{
    if (value >= MAX_PALETTES) {
        tig_debug_println("Range exceeded in art set.");
        value = 0;
    }

    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_LIGHT:
    case TIG_ART_TYPE_FACADE:
        return art_id;
    default:
        return (art_id & ~((MAX_PALETTES - 1) << ART_ID_PALETTE_SHIFT)) | (value << ART_ID_PALETTE_SHIFT);
    }
}

// 0x502E00
int sub_502E00(tig_art_id_t art_id)
{
    int cache_index;
    int palette;

    cache_index = sub_51AA90(art_id);
    if (cache_index == -1) {
        return TIG_ERR_13;
    }

    palette = tig_art_palette(art_id);
    if (tig_art_cache_entries[cache_index].hdr.palette_tbl[palette] == 0) {
        return TIG_ERR_16;
    }

    return TIG_OK;
}

// 0x502E50
int sub_502E50(tig_art_id_t art_id, int x, int y, unsigned int* color_ptr)
{
    TigArtCacheEntry* cache_entry;
    int cache_index;
    int rotation;
    int frame;
    int type;
    int palette;
    uint8_t byte;

    cache_index = sub_51AA90(art_id);
    if (cache_index == -1) {
        return TIG_ERR_13;
    }

    cache_entry = &(tig_art_cache_entries[cache_index]);

    rotation = tig_art_id_rotation_get(art_id);
    frame = tig_art_id_frame_get(art_id);

    if (tig_art_mirroring_enabled) {
        type = tig_art_type(art_id);
        if ((type == TIG_ART_TYPE_CRITTER
                || type == TIG_ART_TYPE_MONSTER
                || type == TIG_ART_TYPE_UNIQUE_NPC)
            && rotation > 0
            && rotation < 4) {
            rotation = MAX_ROTATIONS - rotation;
            x = cache_entry->hdr.frames_tbl[rotation][frame].width - x - 1;
        }
    }

    byte = cache_entry->pixels_tbl[rotation][frame][y * cache_entry->hdr.frames_tbl[rotation][frame].width + x];

    palette = tig_art_palette(art_id);
    if (cache_entry->hdr.palette_tbl[palette] == NULL) {
        palette = 0;
    }

    if (tig_art_bits_per_pixel != 8) {
        switch (tig_art_bits_per_pixel) {
        case 16:
            *color_ptr = *((uint16_t*)cache_entry->hdr.palette_tbl[palette] + byte);
            break;
        case 24:
            *color_ptr = *((uint32_t*)cache_entry->hdr.palette_tbl[palette] + byte);
            break;
        case 32:
            *color_ptr = *((uint32_t*)cache_entry->hdr.palette_tbl[palette] + byte);
            break;
        }
    }

    return TIG_OK;
}

// 0x502FD0
int sub_502FD0(tig_art_id_t art_id, int x, int y)
{
    unsigned int cache_entry_index;
    int rotation;
    int frame;
    int type;
    int index;

    cache_entry_index = sub_51AA90(art_id);
    if (cache_entry_index == -1) {
        return TIG_ERR_13;
    }

    rotation = tig_art_id_rotation_get(art_id);
    frame = tig_art_id_frame_get(art_id);

    if (tig_art_mirroring_enabled) {
        type = tig_art_type(art_id);
        if ((type == TIG_ART_TYPE_CRITTER
                || type == TIG_ART_TYPE_MONSTER
                || type == TIG_ART_TYPE_UNIQUE_NPC)
            && rotation > 0
            && rotation < 4) {
            rotation = MAX_ROTATIONS - rotation;
            x = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].width - x - 1;
        }
    }

    index = y * tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].width + x;
    if (tig_art_cache_entries[cache_entry_index].pixels_tbl[rotation][frame][index] < 2) {
        return TIG_ERR_16;
    }

    return TIG_OK;
}

// 0x5030B0
int tig_art_data(tig_art_id_t art_id, TigArtData* data)
{
    unsigned int cache_index;
    TigArtCacheEntry* cache_entry;
    int palette;

    cache_index = sub_51AA90(art_id);
    if (cache_index == -1) {
        return TIG_ERR_16;
    }

    cache_entry = &(tig_art_cache_entries[cache_index]);

    data->flags = cache_entry->hdr.flags;
    data->fps = cache_entry->hdr.fps;
    data->bpp = tig_art_bits_per_pixel;
    data->num_frames = cache_entry->hdr.num_frames;
    data->action_frame = cache_entry->hdr.action_frame;

    palette = tig_art_palette(art_id);

    data->palette1 = cache_entry->field_254[palette];
    if (data->palette1 == NULL) {
        data->palette1 = cache_entry->field_254[0];
    }

    data->palette2 = cache_entry->hdr.palette_tbl[palette];
    if (data->palette2 == 0) {
        data->palette2 = cache_entry->hdr.palette_tbl[0];
    }

    switch (tig_art_bits_per_pixel) {
    case 8:
        data->color_key = ((uint8_t*)data->palette1)[0];
        break;
    case 16:
        data->color_key = ((uint16_t*)data->palette1)[0];
        break;
    case 24:
        data->color_key = ((uint32_t*)data->palette1)[0];
        break;
    case 32:
        data->color_key = ((uint32_t*)data->palette1)[0];
        break;
    }

    return TIG_OK;
}

// 0x5031C0
int tig_art_frame_data(tig_art_id_t art_id, TigArtFrameData* data)
{
    unsigned int cache_index;
    TigArtCacheEntry* cache_entry;
    int rotation;
    int frame;
    bool mirrored;
    int type;

    cache_index = sub_51AA90(art_id);
    if (cache_index == -1) {
        return TIG_ERR_16;
    }

    rotation = tig_art_id_rotation_get(art_id);
    frame = tig_art_id_frame_get(art_id);
    type = tig_art_type(art_id);

    mirrored = false;
    if (tig_art_mirroring_enabled
        && (type == TIG_ART_TYPE_CRITTER
            || type == TIG_ART_TYPE_MONSTER
            || type == TIG_ART_TYPE_UNIQUE_NPC)
        && rotation > 0
        && rotation < 4) {
        mirrored = true;
        rotation = MAX_ROTATIONS - rotation;
    }

    cache_entry = &(tig_art_cache_entries[cache_index]);

    data->width = cache_entry->hdr.frames_tbl[rotation][frame].width;
    data->height = cache_entry->hdr.frames_tbl[rotation][frame].height;
    data->hot_x = cache_entry->hdr.frames_tbl[rotation][frame].hot_x;
    data->hot_y = cache_entry->hdr.frames_tbl[rotation][frame].hot_y;
    data->offset_x = cache_entry->hdr.frames_tbl[rotation][frame].offset_x;
    data->offset_y = cache_entry->hdr.frames_tbl[rotation][frame].offset_y;

    if ((type == TIG_ART_TYPE_WALL || type == TIG_ART_TYPE_PORTAL)
        && (rotation < 2 || rotation > 5)) {
        data->hot_x -= 40;
        data->hot_y += 20;
    }

    if (mirrored) {
        data->hot_x = data->width - data->hot_x - 1;
        data->offset_x = -data->offset_x;
    }

    if ((sub_504FD0(art_id) & 0x1) != 0) {
        if (type == TIG_ART_TYPE_ROOF) {
            data->hot_x = 0;
            data->offset_x = 0;
        } else {
            data->hot_x = cache_entry->hdr.frames_tbl[rotation][frame].width - data->hot_x - 2;
            data->offset_x = -data->offset_x;
        }
    }

    return TIG_OK;
}

// 0x503340
int sub_503340(tig_art_id_t art_id, uint8_t* dst, int pitch)
{
    unsigned int cache_index;
    int rotation;
    int v2;
    uint8_t* src;
    int width;
    int height;

    cache_index = sub_51AA90(art_id);
    if (cache_index == -1) {
        return TIG_ERR_13;
    }

    rotation = tig_art_id_rotation_get(art_id);
    v2 = tig_art_id_frame_get(art_id);
    src = tig_art_cache_entries[cache_index].pixels_tbl[rotation][v2];
    width = tig_art_cache_entries[cache_index].hdr.frames_tbl[rotation][v2].width;
    height = tig_art_cache_entries[cache_index].hdr.frames_tbl[rotation][v2].height;

    while (height > 0) {
        memcpy(dst, src, width);
        dst += pitch;
        height--;
    }

    return TIG_OK;
}

// 0x5033E0
int sub_5033E0(tig_art_id_t art_id, int a2, int a3)
{
    unsigned int cache_entry_index;
    int rotation;
    int frame;
    int type;
    int delta;
    int v0;
    int v1;
    int v2;
    int v3;
    int v4;

    cache_entry_index = sub_51AA90(art_id);
    if (cache_entry_index == -1) {
        return 0;
    }

    rotation = tig_art_id_rotation_get(art_id);
    frame = tig_art_id_frame_get(art_id);

    // NOTE: Initialize to keep compiler happy.
    delta = 0;

    if (tig_art_mirroring_enabled) {
        delta = 1;

        type = tig_art_type(art_id);
        if ((type == TIG_ART_TYPE_CRITTER
                || type == TIG_ART_TYPE_MONSTER
                || type == TIG_ART_TYPE_UNIQUE_NPC)
            && rotation > 0
            && rotation < 4) {
            delta = -1;
            rotation = MAX_ROTATIONS - rotation;
        }
    }

    v0 = 0;

    if (a2 < 0) {
        a2 = -a2;
    }

    if (a3 < 0) {
        a3 = -a3;
    }

    v1 = 0;
    v2 = 0;
    while ((v1 < a2 || v2 < a3) && v0 < 100) {
        frame++;
        if (frame == tig_art_cache_entries[cache_entry_index].hdr.num_frames) {
            frame = 0;
        }

        v3 = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].offset_x;
        v4 = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].offset_y;

        if (tig_art_mirroring_enabled) {
            v3 *= delta;
            v4 *= delta;
        }

        if (v3 >= 0) {
            v1 += v3;
        } else {
            v1 -= v3;
        }

        if (v4 >= 0) {
            v2 += v4;
        } else {
            v2 -= v4;
        }

        v0++;
    }

    return v0;
}

// 0x503510
int tig_art_size(tig_art_id_t art_id, int* width_ptr, int* height_ptr)
{
    TigArtCacheEntry* art;
    TigArtFileFrameData* frm;
    unsigned int cache_entry_index;
    int type;
    int rotation_offset;
    int num_rotations;
    int rotation;
    int frame;
    int width = 0;
    int height = 0;

    cache_entry_index = sub_51AA90(art_id);
    if (cache_entry_index == -1) {
        return TIG_ERR_16;
    }

    art = &(tig_art_cache_entries[cache_entry_index]);
    type = tig_art_type(art_id);

    if ((art->hdr.flags & TIG_ART_0x01) != 0) {
        rotation_offset = 0;
        num_rotations = 1;
    } else if (tig_art_mirroring_enabled
        && (type == TIG_ART_TYPE_CRITTER
            || type == TIG_ART_TYPE_MONSTER
            || type == TIG_ART_TYPE_UNIQUE_NPC)) {
        rotation_offset = 4;
        num_rotations = 5;
    } else {
        rotation_offset = 0;
        num_rotations = MAX_ROTATIONS;
    }

    for (frame = 0; frame < art->hdr.num_frames; ++frame) {
        for (rotation = 0; rotation < num_rotations; ++rotation) {
            frm = &(art->hdr.frames_tbl[(rotation + rotation_offset) % MAX_ROTATIONS][frame]);
            if (width < frm->width) {
                width = frm->width;
            }
            if (height < frm->height) {
                height = frm->height;
            }
        }
    }

    *width_ptr = width;
    *height_ptr = height;

    return TIG_OK;
}

// 0x503600
int tig_art_tile_id_create(unsigned int a1, unsigned int a2, unsigned int a3, unsigned int a4, unsigned int a5, unsigned int a6, unsigned int a7, unsigned int a8, tig_art_id_t* art_id_ptr)
{
    if (a1 >= 0x40
        || a2 >= 0x40
        || a3 >= 0x10
        || a4 >= 0x10
        || a5 >= 2
        || a6 >= 2
        || a7 >= 2
        || a8 >= 4) {
        return TIG_ERR_12;
    }

    *art_id_ptr = sub_503740((TIG_ART_TYPE_TILE << ART_ID_TYPE_SHIFT)
            | ((a1 & 0x3F) << 22)
            | ((a2 & 0x3F) << 16)
            | ((a5 & 1) << 8)
            | ((a6 & 1) << 7)
            | ((a7 & 1) << 6)
            | ((a8 & 3) << 4),
        a3);

    *art_id_ptr = sub_503800(*art_id_ptr, a4);

    return TIG_OK;
}

// 0x5036B0
int sub_5036B0(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
        return (art_id >> 22) & 0x3F;
    case TIG_ART_TYPE_FACADE:
        return (art_id >> 11) & 0x3F;
    default:
        return 0;
    }
}

// 0x5036E0
int sub_5036E0(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_TILE) {
        return (art_id >> 16) & 0x3F;
    }

    return 0;
}

// 0x503700
int sub_503700(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_TILE) {
        return 0;
    }

    int v1 = (art_id >> 12) & 0xF;
    if ((sub_504FD0(art_id) & 1) != 0) {
        v1 = dword_5BE8C0[v1];
    }
    return v1;
}

// 0x503740
tig_art_id_t sub_503740(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_TILE) {
        return art_id;
    }

    if (value >= 16) {
        tig_debug_println("Range exceeded in art set.");
        value = 0;
    }

    if (value != dword_5BE880[value]) {
        if (sub_503900(art_id)) {
            art_id = sub_502D30(art_id, sub_504FD0(art_id) | 1);
            value = dword_5BE880[value];
        }
    }

    art_id = (art_id & ~0xF000) | (value << 12);

    return art_id;
}

// 0x5037B0
int sub_5037B0(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_TILE) {
        return 0;
    }

    int v1 = (art_id >> 9) & 7;
    int v2 = sub_503700(art_id);
    if (dword_5BE8C0[v2] == dword_5BE880[v2]) {
        if ((sub_504FD0(art_id) & 1) != 0) {
            v1 += 8;
        }
    }
    return v1;
}

// 0x503800
tig_art_id_t sub_503800(tig_art_id_t art_id, int value)
{
    int v1;

    if (tig_art_type(art_id) != TIG_ART_TYPE_TILE) {
        return art_id;
    }

    if (value >= 16) {
        tig_debug_println("Range exceeded in art set.");
        value = 0;
    }

    v1 = sub_503700(art_id);
    if (value < 8) {
        if (dword_5BE8C0[v1] == dword_5BE880[v1]) {
            art_id = sub_502D30(art_id, sub_504FD0(art_id) & ~1);
        }
        art_id = (art_id & ~0xE00) | (value << 9);
    } else {
        if (dword_5BE8C0[v1] == dword_5BE880[v1]) {
            if (sub_503900(art_id)) {
                art_id = sub_502D30(art_id, sub_504FD0(art_id) | 1);
            }
        }
        art_id = (art_id & ~0xE00) | ((value - 8) << 9);
    }

    return art_id;
}

// 0x5038C0
int sub_5038C0(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_TILE) {
        return (art_id >> 8) & 1;
    }

    if (tig_art_type(art_id) == TIG_ART_TYPE_FACADE) {
        return (art_id >> 25) & 1;
    }

    return 0;
}

// 0x503900
int sub_503900(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_FACADE) {
        return (art_id >> 26) & 1;
    }

    if (tig_art_type(art_id) == TIG_ART_TYPE_TILE) {
        if (sub_503950(art_id) != 0 && sub_503990(art_id) != 0) {
            return 1;
        }
    }

    return 0;
}

// 0x503950
int sub_503950(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_TILE) {
        return (art_id >> 7) & 1;
    }

    if (tig_art_type(art_id) == TIG_ART_TYPE_FACADE) {
        return sub_503900(art_id);
    }

    return 0;
}

// 0x503990
int sub_503990(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_TILE) {
        return (art_id >> 6) & 1;
    }

    if (tig_art_type(art_id) == TIG_ART_TYPE_FACADE) {
        return sub_503900(art_id);
    }

    return 0;
}

// 0x5039D0
int tig_art_wall_id_create(unsigned int a1, int a2, int a3, int rotation, unsigned int palette, int a6, tig_art_id_t* art_id_ptr)
{
    if (a1 >= 256
        || a2 >= 46
        || a3 >= 4
        || rotation >= MAX_ROTATIONS
        || palette >= MAX_PALETTES
        || (a6 & ~0x480) != 0) {
        return TIG_ERR_12;
    }

    *art_id_ptr = tig_art_id_rotation_set((TIG_ART_TYPE_WALL << ART_ID_TYPE_SHIFT)
            | ((a2 & 0x3F) << 14)
            | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
            | ((a3 & 3) << 8)
            | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT)
            | a6,
        rotation);

    return TIG_OK;
}

// 0x503A60
int sub_503A60(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_WALL) {
        return (art_id >> 14) & 0x3F;
    }

    return 0;
}

// 0x503A90
tig_art_id_t sub_503A90(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_WALL) {
        return art_id;
    }

    if (value >= 64) {
        tig_debug_println("Range exceeded in art set.");
        value = 0;
    }

    return (art_id & 0xFFF03FFF) | (value << 14);
}

// 0x503AD0
int sub_503AD0(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_WALL) {
        return (art_id >> 20) & 0xFF;
    }

    return 0;
}

// 0x503B00
int sub_503B00(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_WALL) {
        return (art_id >> 8) & 3;
    }

    return 0;
}

// 0x503B30
tig_art_id_t sub_503B30(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_WALL) {
        return art_id;
    }

    if (value >= 5) {
        tig_debug_println("Range exceeded in art set.");
        return art_id;
    }

    return (art_id & 0xFFFFFCFF) | (value << 8);
}

// 0x503B70
int sub_503B70(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_WALL:
        return art_id & 0x480;
    case TIG_ART_TYPE_PORTAL:
        return art_id & 0x200;
    case TIG_ART_TYPE_ITEM:
        return art_id & 0x800;
    default:
        return 0;
    }
}

// 0x503BB0
tig_art_id_t sub_503BB0(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_WALL:
        return (art_id & ~0x480) | value;
    case TIG_ART_TYPE_PORTAL:
        return (art_id & ~0x200) | value;
    case TIG_ART_TYPE_ITEM:
        return (art_id & ~0x800) | value;
    default:
        return art_id;
    }
}

// 0x503C00
int tig_art_critter_id_create(unsigned int a1, int a2, int a3, unsigned int a4, unsigned int a5, int rotation, int a7, int a8, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (a1 >= 2
        || a2 >= 8
        || a3 >= 16
        || a4 >= 2
        || a5 >= 0x20
        || rotation >= MAX_ROTATIONS
        || a7 >= 32
        || a8 >= 16
        || palette >= MAX_PALETTES) {
        return TIG_ERR_12;
    }

    *art_id_ptr = (TIG_ART_TYPE_CRITTER << ART_ID_TYPE_SHIFT)
        | ((a1 & 1) << 27)
        | ((a2 & 7) << 24)
        | ((a3 & 0xF) << 20)
        | ((a4 & 1) << 19)
        | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
        | ((a7 & 0x1F) << 6)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT)
        | (a8 & 0xF);

    return TIG_OK;
}

// 0x503CD0
int tig_art_monster_id_create(int a1, int a2, unsigned int a3, unsigned int a4, int rotation, int a6, int a7, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (a1 >= 32
        || a2 >= 8
        || a3 >= 2
        || a4 >= 0x20
        || rotation >= MAX_ROTATIONS
        || a6 >= 32
        || a7 >= 16
        || palette >= MAX_PALETTES) {
        return TIG_ERR_12;
    }

    *art_id_ptr = (TIG_ART_TYPE_MONSTER << ART_ID_TYPE_SHIFT)
        | ((a1 & 0x1F) << 23)
        | ((a2 & 7) << 20)
        | ((a3 & 1) << 19)
        | ((a4 & 0x1F) << 12)
        | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
        | ((a6 & 0x1F) << 6)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT)
        | (a7 & 0xF);

    return TIG_OK;
}

// 0x503D80
int tig_art_unique_npc_id_create(int a1, unsigned int a2, unsigned int a3, int rotation, int a5, int a6, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (a1 >= 256
        || a2 >= 2
        || a3 >= 0x20
        || rotation >= MAX_ROTATIONS
        || a5 >= 32
        || a6 >= 16
        || palette >= MAX_PALETTES) {
        return TIG_ERR_12;
    }

    *art_id_ptr = (TIG_ART_TYPE_UNIQUE_NPC << ART_ID_TYPE_SHIFT)
        | ((a1 & 0xFF) << 20)
        | ((a2 & 1) << 19)
        | ((a3 & 0x1F) << 14)
        | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
        | ((a5 & 0x1F) << 6)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT)
        | (a6 & 0xF);

    return TIG_OK;
}

// 0x503E20
int sub_503E20(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        return (art_id >> 6) & 0x1F;
    default:
        return 0;
    }
}

// 0x503E50
tig_art_id_t sub_503E50(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        if (value >= 26) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0x7C0) | (value << 6);
    default:
        return art_id;
    }
}

// 0x503EA0
int sub_503EA0(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_CRITTER) {
        return (art_id >> 24) & 7;
    }

    return 0;
}

// 0x503ED0
tig_art_id_t sub_503ED0(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        if (value >= 5) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0x7000000) | (value << 24);
    default:
        return art_id;
    }
}

// 0x503F20
int sub_503F20(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_MONSTER) {
        return (art_id >> 23) & 0x1F;
    }

    return 0;
}

// 0x503F50
int sub_503F50(int a1)
{
    return dword_5BE900[a1];
}

// 0x503F60
int sub_503F60(tig_art_id_t art_id)
{
    int v1;
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
        v1 = tig_art_num(art_id);
        return dword_5BE980[v1];
    case TIG_ART_TYPE_MONSTER:
        v1 = sub_503F20(art_id);
        return dword_5BE994[v1];
    default:
        return 1;
    }
}

// 0x503FB0
int sub_503FB0(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_CRITTER) {
        return (art_id >> 27) & 1;
    } else {
        return 0;
    }
}

// 0x503FE0
tig_art_id_t sub_503FE0(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        if (value >= 2) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0x8000000) | (value << 27);
    default:
        return art_id;
    }
}

// 0x504030
int sub_504030(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
        return (art_id >> 20) & 0xF;
    case TIG_ART_TYPE_MONSTER:
        return (art_id >> 20) & 7;
    default:
        return 0;
    }
}

// 0x504060
tig_art_id_t sub_504060(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
        if (value >= 9) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0xF00000) | (value << 20);
    case TIG_ART_TYPE_MONSTER:
        if (value >= 8) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0x700000) | (value << 20);
    default:
        return art_id;
    }
}

// 0x5040D0
int sub_5040D0(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        return art_id & 0xF;
    default:
        return 0;
    }
}

// 0x504100
tig_art_id_t sub_504100(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        if (value >= 15) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0xF) | value;
    default:
        return art_id;
    }
}

// 0x504150
int sub_504150(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        return (art_id >> 19) & 1;
    default:
        return 0;
    }
}

// 0x504180
tig_art_id_t sub_504180(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        if (value >= 2) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0x80000) | (value << 19);
    default:
        return art_id;
    }
}

// 0x5041D0
int tig_art_portal_id_create(unsigned int a1, int a2, int a3, unsigned int a4, int rotation, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (a1 >= 0x200
        || a2 >= 2
        || a4 >= 0x20
        || rotation >= MAX_ROTATIONS
        || palette >= MAX_PALETTES
        || (a3 & ~0x200u) != 0) {
        return TIG_ERR_12;
    }

    *art_id_ptr = (TIG_ART_TYPE_PORTAL << ART_ID_TYPE_SHIFT)
        | ((a1 & 0x1FF) << 19)
        | ((a4 & 0x1F) << 14)
        | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
        | ((a2 & 1) << 10)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT)
        | a3;

    return TIG_OK;
}

// 0x504260
int sub_504260(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_PORTAL) {
        return (art_id >> 10) & 1;
    } else {
        return 0;
    }
}

// 0x504290
int tig_art_scenery_id_create(unsigned int a1, int a2, unsigned int a3, int rotation, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (a1 >= 0x200
        || a2 >= 32
        || a3 >= 0x20
        || rotation >= MAX_ROTATIONS
        || palette >= MAX_PALETTES) {
        return TIG_ERR_12;
    }

    *art_id_ptr = (TIG_ART_TYPE_SCENERY << ART_ID_TYPE_SHIFT)
        | ((a1 & 0x1FF) << 19)
        | ((a3 & 0x1F) << 14)
        | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
        | ((a2 & 0x1F) << 6)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT);

    return TIG_OK;
}

// 0x504300
int sub_504300(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_SCENERY) {
        return (art_id >> 6) & 0x1F;
    } else {
        return 0;
    }
}

// 0x504330
int tig_art_interface_id_create(unsigned int num, unsigned int a2, unsigned char a3, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (num >= 0x1000
        || a2 >= 0x100
        || palette >= MAX_PALETTES) {
        return TIG_ERR_12;
    }

    *art_id_ptr = (TIG_ART_TYPE_INTERFACE << ART_ID_TYPE_SHIFT)
        | ((num & 0xFFF) << 16)
        | ((a2 & 0xFF) << 8)
        | ((a3 & 1) << 7)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT);

    return TIG_OK;
}

// 0x504390
int sub_504390(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_INTERFACE) {
        return (art_id >> 7) & 1;
    } else {
        return 0;
    }
}

// 0x5043C0
int tig_art_item_id_create(int a1, int a2, int a3, int a4, int a5, int a6, int a7, unsigned int a8, tig_art_id_t* art_id_ptr)
{
    // NOTE: Unsigned compare
    if ((unsigned int)a1 >= 0x3E8
        || a2 >= 4
        || (a3 & ~0x800) != 0
        || (a4 & ~0x400) != 0
        || a5 >= 16
        || a6 >= 16
        || a7 >= 8
        || a8 >= 4) {
        return TIG_ERR_12;
    }

    *art_id_ptr = (TIG_ART_TYPE_ITEM << ART_ID_TYPE_SHIFT)
        | ((a1 & 0x7FF) << 17)
        | ((a7 & 7) << 14)
        | ((a2 & 3) << 12)
        | ((a5 & 0xF) << 6)
        | ((a8 & 3) << 4)
        | (a6 & 0xF)
        | a3
        | a4;

    // NOTE: Signed compare.
    if ((int)a1 >= (int)sub_502830(*art_id_ptr)) {
        return TIG_ERR_12;
    }

    return TIG_OK;
}

// 0x504490
int sub_504490(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_ITEM) {
        return (art_id >> 12) & 3;
    } else {
        return 0;
    }
}

// 0x5044C0
tig_art_id_t sub_5044C0(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_ITEM) {
        if (value >= 4) {
            value = 0;
        }
        return (art_id & ~0x3000) | (value << 12);
    } else {
        return 0;
    }
}

// 0x5044F0
int sub_5044F0(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_ITEM) {
        return (art_id >> 6) & 0x1F;
    } else {
        return 0;
    }
}

// 0x504520
tig_art_id_t sub_504520(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ITEM) {
        return art_id;
    }

    if (value >= 16) {
        return art_id;
    }

    return (art_id & ~0x3C0) | (value << 6);
}

// 0x504550
int sub_504550(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_ITEM) {
        return art_id & 0xF;
    } else {
        return 0;
    }
}

// 0x504570
int sub_504570(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_ITEM) {
        return (art_id >> 14) & 7;
    } else {
        return 0;
    }
}

// 0x5045A0
int sub_5045A0(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_ITEM) {
        return art_id & 0x400;
    } else {
        return 0;
    }
}

// 0x5045C0
tig_art_id_t sub_5045C0(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ITEM) {
        return art_id;
    }

    if (value) {
        art_id |= 0x400;
    } else {
        art_id &= ~0x400;
    }

    return art_id;
}

// 0x5045F0
int tig_art_container_id_create(unsigned int a1, int a2, unsigned int a3, int rotation, unsigned int a5, tig_art_id_t* art_id_ptr)
{
    if (a1 >= 0x200
        || a2 >= 32
        || a3 >= 0x20
        || rotation >= MAX_ROTATIONS
        || a5 >= 4) {
        return TIG_ERR_12;
    }

    *art_id_ptr = (TIG_ART_TYPE_CONTAINER << ART_ID_TYPE_SHIFT)
        | ((a1 & 0x1FF) << 19)
        | ((a3 & 0x1F) << 14)
        | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
        | ((a2 & 0x1F) << 6)
        | ((a5 & 3) << 4);

    return TIG_OK;
}

// 0x504660
int sub_504660(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_CONTAINER) {
        return (art_id >> 6) & 0x1F;
    } else {
        return 0;
    }
}

// 0x504690
int tig_art_light_id_create(unsigned int a1, unsigned int a2, unsigned int rotation, int a4, tig_art_id_t* art_id_ptr)
{
    if (a1 >= 512
        || a2 >= 128
        || (a4 ? rotation >= 32 : rotation >= 8)) {
        return TIG_ERR_12;
    }

    *art_id_ptr = (TIG_ART_TYPE_LIGHT << ART_ID_TYPE_SHIFT)
        | ((a1 & 0x1FF) << 19)
        | ((a2 & 0x7F) << 12)
        | ((rotation & (MAX_ROTATIONS - 1)) << LIGHT_ID_ROTATION_SHIFT)
        | ((rotation & 0x1F) << 4)
        | (a4 & 1);

    return TIG_OK;
}

// 0x504700
int sub_504700(tig_art_id_t art_id)
{
    if (sub_504790(art_id) != 0) {
        return (art_id >> 4) & 0x1F;
    } else {
        return tig_art_id_rotation_get(art_id);
    }
}

// 0x504730
tig_art_id_t sub_504730(tig_art_id_t art_id, int rotation)
{
    if (!sub_504790(art_id)) {
        return tig_art_id_rotation_set(art_id, rotation);
    }

    if (rotation >= 32) {
        rotation = 0;
    } else if (rotation < 0) {
        rotation = 31;
    }

    // TODO: Check.
    return (art_id & ~0xFF0)
        | ((rotation % 8) << 9)
        | ((rotation & 0xF) << 4);
}

// 0x504790
int sub_504790(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_LIGHT) {
        return art_id & 1;
    } else {
        return 0;
    }
}

// 0x5047B0
int tig_art_roof_id_create(unsigned int a1, int a2, unsigned int a3, unsigned int a4, tig_art_id_t* art_id_ptr)
{
    int v1;
    int v2;

    if (a1 >= 0x200
        || a2 >= 13
        || a3 > 1
        || a4 > 1) {
        return TIG_ERR_12;
    }

    if (a2 >= 9) {
        a2 -= 9;
        v1 = 1;
        v2 = 1;
    } else {
        v1 = 0;
        v2 = 0;
    }

    *art_id_ptr = sub_502D30((TIG_ART_TYPE_ROOF << ART_ID_TYPE_SHIFT)
            | ((a1 & 0x1FF) << 19)
            | ((a2 & 0x1F) << 14)
            | ((a3 & 1) << 13)
            | ((a4 & 1) << 12)
            | ((v2 & 3) << 4),
        v1);

    return TIG_OK;
}

// 0x504840
int sub_504840(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_ROOF) {
        int v1 = tig_art_id_frame_get(art_id);
        if ((sub_504FD0(art_id) & 1) != 0) {
            v1 += 9;
        }
        return v1;
    } else {
        return -1;
    }
}

// 0x504880
tig_art_id_t sub_504880(tig_art_id_t art_id, int frame)
{
    int v0 = 0;
    int palette = 0;

    if (tig_art_type(art_id) != TIG_ART_TYPE_ROOF) {
        return art_id;
    }

    if (frame >= 9) {
        frame = frame - 9;
        v0 = 1;
        palette = 1;
    }

    art_id = tig_art_id_frame_set(art_id, frame);
    art_id = sub_502D30(art_id, v0);
    art_id = tig_art_set_palette(art_id, palette);

    return art_id;
}

// 0x5048D0
int sub_5048D0(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_ROOF) {
        return (art_id >> 13) & 1;
    } else {
        return 0;
    }
}

// 0x504900
tig_art_id_t sub_504900(tig_art_id_t art_id, unsigned int value)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_ROOF) {
        if (value > 1) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0x2000) | (value << 13);
    } else {
        return art_id;
    }
}

// 0x504940
int sub_504940(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_ROOF) {
        return (art_id >> 12) & 1;
    } else {
        return 0;
    }
}

// 0x504970
tig_art_id_t sub_504970(tig_art_id_t art_id, unsigned int value)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_ROOF) {
        if (value > 1) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0x1000) | (value << 12);
    } else {
        return art_id;
    }
}

// 0x5049B0
int tig_art_facade_id_create(unsigned int a1, unsigned int a2, unsigned int a3, unsigned int a4, unsigned int a5, unsigned int a6, tig_art_id_t* art_id_ptr)
{
    if (a1 >= 0x200
        || a2 >= 0x40
        || a3 >= 2
        || a4 >= 2
        || a5 >= 0x400
        || a6 >= 2) {
        return TIG_ERR_12;
    }

    *art_id_ptr = (TIG_ART_TYPE_FACADE << ART_ID_TYPE_SHIFT)
        | (((a1 < 0x100 ? 0 : 1) & 1) << 27)
        | ((a4 & 1) << 26)
        | ((a3 & 1) << 25)
        | ((a1 & 0xFF) << 17)
        | ((a2 & 0x3F) << 11)
        | ((a5 & 0x3FF) << 1)
        | (a6 & 1);

    return TIG_OK;
}

// 0x504A60
int sub_504A60(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_FACADE) {
        int v1 = (art_id >> 17) & 0xFF;
        if ((art_id & 0x8000000) != 0) {
            v1 += 256;
        }
        return v1;
    } else {
        return 0;
    }
}

// 0x504A90
int sub_504A90(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_FACADE) {
        return (art_id >> 1) & 0x3FF;
    } else {
        return 0;
    }
}

// 0x504AC0
int sub_504AC0(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_FACADE) {
        return art_id & 1;
    } else {
        return 0;
    }
}

// 0x504AE0
int tig_art_eye_candy_id_create(unsigned int a1, unsigned int a2, int rotation, int translucency, int type, unsigned int palette, int scale, tig_art_id_t* art_id)
{
    if (a1 >= 512) {
        return TIG_ERR_12;
    }

    if (a2 >= 128) {
        return TIG_ERR_12;
    }

    if (rotation >= MAX_ROTATIONS) {
        return TIG_ERR_12;
    }

    if (translucency >= 2) {
        return TIG_ERR_12;
    }

    if (type >= 4) {
        return TIG_ERR_12;
    }

    if (palette >= MAX_PALETTES) {
        return TIG_ERR_12;
    }

    if (scale < 0 || scale > 7) {
        return TIG_ERR_12;
    }

    // TODO: Check.
    *art_id = (TIG_ART_TYPE_EYE_CANDY << ART_ID_TYPE_SHIFT)
        | ((a1 & 0x1FF) << 19)
        | ((a2 & 0x7F) << 12)
        | ((rotation & (MAX_ROTATIONS - 1)) << EYE_CANDY_ID_ROTATION_SHIFT)
        | ((translucency & 1) << 8)
        | ((type & 3) << 6)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT)
        | ((scale & 7) << 1);

    return TIG_OK;
}

// 0x504B90
int tig_art_eye_candy_id_type_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_EYE_CANDY) {
        return (art_id >> 6) & 3;
    } else {
        return 0;
    }
}

// 0x504BC0
tig_art_id_t tig_art_eye_candy_id_type_set(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_EYE_CANDY) {
        if (value >= 3) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        // NOTE: Rare case - value is additionally ANDed with max value.
        return (art_id & ~0xC0) | ((value & 3) << 6);
    } else {
        return art_id;
    }
}

// 0x504C00
int tig_art_eye_candy_id_translucency_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_EYE_CANDY) {
        return (art_id >> 8) & 1;
    } else {
        return 0;
    }
}

// 0x504C30
tig_art_id_t tig_art_eye_candy_id_translucency_set(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_EYE_CANDY) {
        return (art_id & ~0x100) | ((value & 1) << 8);
    } else {
        return art_id;
    }
}

// 0x504C60
int tig_art_eye_candy_id_scale_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_EYE_CANDY) {
        return (art_id >> 1) & 7;
    } else {
        // NOTE: Rare case - default is not 0.
        return 4;
    }
}

// 0x504C90
tig_art_id_t tig_art_eye_candy_id_scale_set(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_EYE_CANDY) {
        return (art_id & ~0xE) | ((value & 7) << 1);
    } else {
        return art_id;
    }
}

// 0x504CC0
bool sub_504CC0(const char* name)
{
    TigArtHeader hdr;
    FILE* stream;
    int num_rotations;
    int rotation;
    int palette;
    char path[_MAX_PATH];
    uint32_t palette_entries[MAX_PALETTES][256];

    for (rotation = 0; rotation < MAX_ROTATIONS; ++rotation) {
        hdr.frames_tbl[rotation] = NULL;
        hdr.pixels_tbl[rotation] = NULL;
    }

    sprintf(path, "%s.art", name);

    stream = fopen(path, "rb");
    if (stream == NULL) {
        return false;
    }

    if (fread(&hdr, sizeof(hdr), 1, stream) != 1) {
        fclose(stream);
        return false;
    }

    // Only 8-bpp palette-indexed ART files are supported.
    if (hdr.bpp != 8) {
        fclose(stream);
        return false;
    }

    for (palette = 0; palette < MAX_PALETTES; ++palette) {
        if (fread(palette_entries[palette], sizeof(uint32_t), 256, stream) != 256) {
            fclose(stream);
            return false;
        }
    }

    num_rotations = (hdr.flags & TIG_ART_0x01) != 0 ? 1 : MAX_ROTATIONS;

    for (rotation = 0; rotation < num_rotations; ++rotation) {
        hdr.frames_tbl[rotation] = (TigArtFileFrameData*)MALLOC(sizeof(TigArtFileFrameData) * hdr.num_frames);
        hdr.pixels_tbl[rotation] = (uint8_t*)MALLOC(hdr.data_size[rotation]);
    }

    for (; rotation < MAX_ROTATIONS; ++rotation) {
        hdr.frames_tbl[rotation] = hdr.frames_tbl[0];
        hdr.pixels_tbl[rotation] = hdr.pixels_tbl[0];
    }

    for (rotation = 0; rotation < num_rotations; ++rotation) {
        if (fread(hdr.frames_tbl[rotation], sizeof(TigArtFileFrameData), hdr.num_frames, stream) != (size_t)hdr.num_frames) {
            fclose(stream);
            return false;
        }
    }

    for (rotation = 0; rotation < num_rotations; ++rotation) {
        if (fread(hdr.pixels_tbl[rotation], hdr.data_size[rotation], 1, stream) != 1) {
            fclose(stream);
            return false;
        }
    }

    fclose(stream);

    stream = fopen(path, "wb");
    if (stream == NULL) {
        return false;
    }

    hdr.num_frames = 1;
    hdr.action_frame = 0;

    for (rotation = 0; rotation < num_rotations; ++rotation) {
        hdr.data_size[rotation] = hdr.frames_tbl[rotation]->data_size;
    }

    if (fwrite(&hdr, sizeof(hdr), 1, stream) != 1) {
        fclose(stream);
        return false;
    }

    for (palette = 0; palette < MAX_PALETTES; ++palette) {
        if (fwrite(palette_entries[palette], sizeof(uint32_t), 256, stream) != 256) {
            fclose(stream);
            return false;
        }
    }

    for (rotation = 0; rotation < num_rotations; ++rotation) {
        if (fwrite(hdr.frames_tbl[rotation], sizeof(TigArtFileFrameData), hdr.num_frames, stream) != (size_t)hdr.num_frames) {
            fclose(stream);
            return false;
        }
    }

    for (rotation = 0; rotation < num_rotations; ++rotation) {
        if (fwrite(hdr.pixels_tbl[rotation], hdr.data_size[rotation], 1, stream) != 1) {
            fclose(stream);
            return false;
        }
    }

    fclose(stream);

    for (rotation = 0; rotation < num_rotations; ++rotation) {
        FREE(hdr.frames_tbl[rotation]);
        FREE(hdr.pixels_tbl[rotation]);
    }

    return true;
}

// 0x504FD0
int sub_504FD0(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_PORTAL:
    case TIG_ART_TYPE_ROOF:
        return art_id & 0xF;
    default:
        return 0;
    }
}

// 0x505000
void sub_505000(tig_art_id_t art_id, TigPalette src_palette, TigPalette dst_palette)
{
    TigPaletteModifyInfo modify_info;

    if (dword_604744 != NULL && dword_604744(art_id, &modify_info)) {
        modify_info.src_palette = src_palette;
        modify_info.dst_palette = dst_palette;
        tig_palette_modify(&modify_info);
    } else {
        tig_palette_copy(dst_palette, src_palette);
    }
}

// 0x505060
int art_get_video_buffer(unsigned int cache_entry_index, tig_art_id_t art_id, TigVideoBuffer** video_buffer_ptr)
{
    TigVideoBufferCreateInfo vb_create_info;
    TigArtBlitSpec blt;
    TigRect rect;
    int type;
    int palette;
    int rotation;
    int frame;
    int rc;
    art_size_t system_memory_size = 0;
    art_size_t video_memory_size = 0;

    if (sub_51F860() != TIG_OK) {
        return TIG_ERR_16;
    }

    type = tig_art_type(art_id);
    switch (type) {
    case TIG_ART_TYPE_ITEM:
        if (sub_504490(art_id) != 0) {
            return TIG_ERR_16;
        }
        break;
    case TIG_ART_TYPE_INTERFACE:
        if (sub_504390(art_id) == 0) {
            return TIG_ERR_16;
        }
        break;
    case TIG_ART_TYPE_MISC:
        return TIG_ERR_16;
    case TIG_ART_TYPE_LIGHT:
        if (sub_504790(art_id) == 0) {
            return TIG_ERR_16;
        }
        break;
    case TIG_ART_TYPE_ROOF:
        // TODO: Incomplete.
        break;
    case TIG_ART_TYPE_TILE:
        art_id = sub_502D30(art_id, sub_504FD0(art_id) & ~1);
        break;
    }

    rotation = tig_art_id_rotation_get(art_id);
    if (tig_art_mirroring_enabled
        && (type == TIG_ART_TYPE_CRITTER
            || type == TIG_ART_TYPE_MONSTER
            || type == TIG_ART_TYPE_UNIQUE_NPC)
        && rotation > 0 && rotation < 4) {
        tig_debug_printf("Error in art_get_video_buffer() - requested rotation %d for mirrored object\n", rotation);
    }

    if ((type == TIG_ART_TYPE_WALL
            || type == TIG_ART_TYPE_PORTAL)
        && (rotation & 1) != 0) {
        art_id = tig_art_id_rotation_set(art_id, rotation - 1);
    }

    palette = tig_art_palette(art_id);

    if (tig_art_cache_entries[cache_entry_index].field_254[palette] == NULL) {
        palette = 0;
        art_id = tig_art_set_palette(art_id, 0);
    }

    if (tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation] == NULL) {
        system_memory_size = sizeof(TigVideoBuffer*) * tig_art_cache_entries[cache_entry_index].hdr.num_frames;
        tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation] = (TigVideoBuffer**)MALLOC(system_memory_size);

        vb_create_info.color_key = tig_color_make(0, 0, 0);
        vb_create_info.background_color = vb_create_info.color_key;

        if (dword_604718) {
            vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_TEXTURE;
            vb_create_info.color_key = 0;
        } else {
            vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_COLOR_KEY | TIG_VIDEO_BUFFER_CREATE_VIDEO_MEMORY;
        }

        for (frame = 0; frame < tig_art_cache_entries[cache_entry_index].hdr.num_frames; frame++) {
            vb_create_info.width = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].width;
            vb_create_info.height = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].height;
            video_memory_size += vb_create_info.width * vb_create_info.height * tig_art_bytes_per_pixel;
            rc = tig_video_buffer_create(&vb_create_info, &(tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame]));
            if (rc != TIG_OK) {
                while (frame > 0) {
                    tig_video_buffer_destroy(tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame - 1]);
                    frame--;
                }

                FREE(tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation]);
                tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation] = NULL;

                return rc;
            }
        }

        tig_art_cache_entries[cache_entry_index].field_194[palette][rotation] = 1;
    }

    if (tig_art_cache_entries[cache_entry_index].field_194[palette][rotation] == 1) {
        rect.x = 0;
        rect.y = 0;

        blt.flags = 0;
        blt.src_rect = &rect;
        blt.dst_rect = &rect;

        for (frame = 0; frame < tig_art_cache_entries[cache_entry_index].hdr.num_frames; frame++) {
            rect.width = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].width;
            rect.height = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].height;

            blt.art_id = tig_art_id_frame_set(art_id, frame);
            blt.dst_video_buffer = tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame];

            if (dword_604718) {
                rc = sub_5059F0(art_id, &blt);
            } else {
                rc = art_blit(art_id, &blt);
            }
        }

        tig_art_cache_entries[cache_entry_index].field_194[palette][rotation] = 0;
    }

    frame = tig_art_id_frame_get(art_id);

    *video_buffer_ptr = tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame];

    tig_art_cache_entries[cache_entry_index].system_memory_usage += system_memory_size;
    tig_art_cache_entries[cache_entry_index].video_memory_usage += video_memory_size;

    tig_art_available_system_memory -= system_memory_size;
    tig_art_available_video_memory -= video_memory_size;

    return TIG_OK;
}

// 0x505940
int sub_505940(unsigned int art_blt_flags, unsigned int* vb_blt_flags_ptr)
{
    if ((art_blt_flags & 0x1802C) != 0) {
        return TIG_ERR_12;
    }

    *vb_blt_flags_ptr = 0;

    if ((art_blt_flags & TIG_ART_BLT_MIRROR_LEFT_RIGHT) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_MIRROR_LEFT_RIGHT;
    }

    if ((art_blt_flags & TIG_ART_BLT_MIRROR_UP_DOWN) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_MIRROR_UP_DOWN;
    }

    if ((art_blt_flags & TIG_ART_BLT_0x10) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_0x0004;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_MODE_0x40) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_0x0010;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_MODE_0x80) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_0x0020;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_MODE_0x100) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_0x0040;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_MODE_0x200) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_0x0080;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_MODE_GRADIENT_HORIZONTAL) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_0x0100;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_MODE_GRADIENT_VERTICAL) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_0x0100;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_MODE_GRADIENT_CORNERS) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_0x0100;
    }

    if ((art_blt_flags & TIG_ART_BLT_0x2000) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_0x0200;
    }

    if ((art_blt_flags & TIG_ART_BLT_0x4000) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_0x0400;
    }

    if ((art_blt_flags & TIG_ART_BLT_0x20000) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_0x0400;
    }

    return TIG_OK;
}

// 0x5059F0
int sub_5059F0(int cache_entry_index, TigArtBlitSpec* blt)
{
    TigVideoBufferData video_buffer_data;
    TigPalette plt;
    int rc;
    int rotation;
    int frame;
    int palette;
    uint8_t* src_pixels;
    int width;
    int height;
    int delta;
    uint8_t* dst_pixels;
    int dst_skip;
    int x;
    int y;
    unsigned int color;

    rc = tig_video_buffer_lock(blt->dst_video_buffer);
    if (rc != TIG_OK) {
        return rc;
    }

    rc = tig_video_buffer_data(blt->dst_video_buffer, &video_buffer_data);
    if (rc != TIG_OK) {
        tig_video_buffer_unlock(blt->dst_video_buffer);
        return rc;
    }

    rotation = tig_art_id_rotation_get(blt->art_id);
    frame = tig_art_id_frame_get(blt->art_id);
    palette = tig_art_palette(blt->art_id);

    src_pixels = tig_art_cache_entries[cache_entry_index].pixels_tbl[rotation][frame];
    width = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].width;
    height = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].height;

    plt = tig_art_cache_entries[cache_entry_index].hdr.palette_tbl[palette];
    if (plt == NULL) {
        plt = tig_art_cache_entries[cache_entry_index].hdr.palette_tbl[0];
    }

    if ((sub_504FD0(blt->art_id) & 1) != 0) {
        src_pixels = src_pixels + width * height - 1;
        delta = -1;
    } else {
        delta = 1;
    }

    switch (tig_art_bits_per_pixel) {
    case 8:
        break;
    case 16:
        if (delta > 0) {
            dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
            dst_skip = video_buffer_data.pitch / 2 - width;
        } else {
            dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 2 * (height - 1);
            dst_skip = -width - video_buffer_data.pitch / 2;
        }
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                if (*src_pixels != 0) {
                    color = tig_color_16_to_32(*((uint16_t*)plt + *src_pixels));
                } else {
                    color = 0;
                }
                *(uint16_t*)dst_pixels = (uint16_t)color;
                src_pixels += delta;
                dst_pixels += 2;
            }
            dst_pixels += dst_skip;
        }
        break;
    case 24:
        if (delta > 0) {
            dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
            dst_skip = video_buffer_data.pitch / 3 - width;
        } else {
            dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 3 * (height - 1);
            dst_skip = -width - video_buffer_data.pitch / 3;
        }
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                if (*src_pixels != 0) {
                    color = tig_color_16_to_32(*((uint32_t*)plt + *src_pixels));
                } else {
                    color = 0;
                }
                dst_pixels[0] = (uint8_t)color;
                dst_pixels[1] = (uint8_t)(color >> 8);
                dst_pixels[2] = (uint8_t)(color >> 16);
                src_pixels += delta;
                dst_pixels += 3;
            }
            dst_pixels += dst_skip;
        }
        break;
    case 32:
        if (delta > 0) {
            dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
            dst_skip = video_buffer_data.pitch / 4 - width;
        } else {
            dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
            dst_skip = -width - video_buffer_data.pitch / 4;
        }
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                if (*src_pixels != 0) {
                    color = tig_color_16_to_32(*((uint32_t*)plt + *src_pixels));
                } else {
                    color = 0;
                }
                *(uint32_t*)dst_pixels = (uint32_t)color;
                src_pixels += delta;
                dst_pixels += 4;
            }
            dst_pixels += dst_skip;
        }
        break;
    }

    tig_video_buffer_unlock(blt->dst_video_buffer);

    return TIG_OK;
}

static int __ftol(double a1)
{
    return (int)a1;
}

// 0x505EB0
int art_blit(int cache_entry_index, TigArtBlitSpec* blt)
{
    TigVideoBufferData video_buffer_data;
    TigArtCacheEntry* art;
    TigArtFileFrameData* frm;
    TigPalette plt;
    TigRect bounds;
    TigRect src_rect;
    TigRect dst_rect;
    TigRect tmp_rect;
    int rc;
    int rotation;
    int frame;
    int palette;
    uint8_t* src_pixels;
    int width;
    int height;
    int delta;
    uint8_t* dst_pixels;
    int dst_skip;
    int x;
    int y;
    bool stretched;
    float width_ratio;
    float height_ratio;
    // unsigned int color;
    float start_opacity_vertical_step;
    float end_opacity_vertical_step;
    float start_opacity_x;
    float end_opacity_x;
    float opacity_range;
    float start_opacity;
    float end_opacity;
    float current_opacity;
    float opacity_horizontal_step;
    uint32_t* mask;

    rc = tig_video_buffer_lock(blt->dst_video_buffer);
    if (rc != TIG_OK) {
        return rc;
    }

    rc = tig_video_buffer_data(blt->dst_video_buffer, &video_buffer_data);
    if (rc != TIG_OK) {
        tig_video_buffer_unlock(blt->dst_video_buffer);
        return rc;
    }

    rotation = tig_art_id_rotation_get(blt->art_id);
    frame = tig_art_id_frame_get(blt->art_id);

    art = &(tig_art_cache_entries[cache_entry_index]);
    frm = &(art->hdr.frames_tbl[rotation][frame]);

    if (blt->src_rect->width == blt->dst_rect->width
        && blt->src_rect->height == blt->dst_rect->height) {
        stretched = false;
    } else {
        stretched = true;
        width_ratio = (float)blt->src_rect->width / (float)blt->dst_rect->width;
        height_ratio = (float)blt->dst_rect->height / (float)blt->dst_rect->height;
    }

    src_pixels = art->pixels_tbl[rotation][frame];
    width = frm->width;
    height = frm->height;

    bounds.x = 0;
    bounds.y = 0;
    bounds.width = width;
    bounds.height = height;

    if (tig_rect_intersection(blt->src_rect, &bounds, &src_rect) != TIG_OK) {
        // Specified source rectangle is out of bounds of the frame, there is
        // nothing to blit.
        tig_video_buffer_unlock(blt->dst_video_buffer);
        return TIG_OK;
    }

    tmp_rect = *blt->dst_rect;

    if (stretched) {
        // TODO: Incomplete.
    } else {
        tmp_rect.x += src_rect.x - blt->src_rect->x;
        tmp_rect.y += src_rect.y - blt->src_rect->y;
        tmp_rect.width += src_rect.width - blt->src_rect->width;
        tmp_rect.height += src_rect.height - blt->src_rect->height;
    }

    bounds.x = 0;
    bounds.y = 0;
    bounds.width = video_buffer_data.width;
    bounds.height = video_buffer_data.height;

    if (tig_rect_intersection(&tmp_rect, &bounds, &dst_rect) != TIG_OK) {
        // Specified destination rectangle is out of bounds of destination
        // video buffer bounds, there is nothing to blit.
        tig_video_buffer_unlock(blt->dst_video_buffer);
        return TIG_OK;
    }

    if (stretched) {
        // TODO: Incomplete.
    } else {
        src_rect.x += dst_rect.x - tmp_rect.x;
        src_rect.y += dst_rect.y - tmp_rect.y;
        src_rect.width += dst_rect.width - tmp_rect.width;
        src_rect.height += dst_rect.width - tmp_rect.height;
    }

    if ((blt->flags & TIG_ART_BLT_PALETTE_CUSTOM) != 0) {
        plt = blt->field_C;
    } else if ((blt->flags & TIG_ART_BLT_PALETTE_NORMAL) != 0) {
        palette = tig_art_palette(blt->art_id);
        plt = art->hdr.palette_tbl[palette];
        if (plt == NULL) {
            plt = art->hdr.palette_tbl[0];
        }
    } else {
        palette = tig_art_palette(blt->art_id);
        plt = art->field_254[palette];
        if (plt == NULL) {
            plt = art->field_254[0];
        }
    }

    // TODO: Incomplete.

    int src_checkerboard_cur_x = src_rect.x;
    int src_checkerboard_cur_y = src_rect.y;
    int dst_checkerboard_cur_x = dst_rect.x;
    int dst_checkerboard_cur_y = dst_rect.y;

    if ((sub_504FD0(blt->art_id) & 1) != 0) {
        src_pixels = src_pixels + width * height - 1;
        delta = -1;
    } else {
        delta = 1;
    }

    if (stretched) {
        // TODO: Incomplete.
    } else {
        if ((blt->flags & TIG_ART_BLT_0x2000) != 0) {
            // 0x5084E9
            if ((blt->flags & TIG_ART_BLT_0x10) != 0) {
                // 0x5084F5
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mult(*((uint32_t*)plt + *src_pixels), blt->field_10);
                                *(uint32_t*)dst_pixels = tig_color_plus(*(uint32_t*)dst_pixels, color);
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x20) != 0) {
                // 0x508A1C
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mult(*((uint32_t*)plt + *src_pixels), blt->field_10);
                                if ((*(uint32_t*)dst_pixels & tig_color_red_mask) < (color & tig_color_red_mask)) {
                                    *(uint32_t*)dst_pixels = (*(uint32_t*)dst_pixels & ~tig_color_red_mask) | (color & tig_color_red_mask);
                                }
                                if ((*(uint32_t*)dst_pixels & tig_color_green_mask) < (color & tig_color_green_mask)) {
                                    *(uint32_t*)dst_pixels = (*(uint32_t*)dst_pixels & ~tig_color_green_mask) | (color & tig_color_green_mask);
                                }
                                if ((*(uint32_t*)dst_pixels & tig_color_blue_mask) < (color & tig_color_blue_mask)) {
                                    *(uint32_t*)dst_pixels = (*(uint32_t*)dst_pixels & ~tig_color_blue_mask) | (color & tig_color_blue_mask);
                                }
                                *(uint32_t*)dst_pixels -= color;
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x40) != 0) {
                // 0x508FB6
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mult(*((uint32_t*)plt + *src_pixels), blt->field_10);
                                *(uint32_t*)dst_pixels = tig_color_mult(color, *(uint32_t*)dst_pixels);
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x80) != 0) {
                // 0x50964F
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mult(*((uint32_t*)plt + *src_pixels), blt->field_10);
                                *(uint32_t*)dst_pixels = tig_color_blend(color,
                                    *(uint32_t*)dst_pixels,
                                    tig_color_rgb_to_grayscale(color));
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x100) != 0) {
                // 0x509CD6
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mult(*((uint32_t*)plt + *src_pixels), blt->field_10);
                                *(uint32_t*)dst_pixels = tig_color_blend(color, *(uint32_t*)dst_pixels, blt->opacity[0]);
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x200) != 0) {
                // 0x50ABD1
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color1 = *((uint32_t*)plt + *src_pixels);
                                uint32_t color2 = *(uint32_t*)dst_pixels;
                                uint32_t color3 = blt->field_10;

                                unsigned int red2 = (color2 & tig_color_red_mask);
                                unsigned int green2 = (color2 & tig_color_green_mask);
                                unsigned int blue2 = (color2 & tig_color_blue_mask);
                                unsigned int red3 = (color3 & tig_color_red_mask);
                                unsigned int green3 = (color3 & tig_color_green_mask);
                                unsigned int blue3 = (color3 & tig_color_blue_mask);
                                unsigned int v1 = tig_color_red_platform_to_rgb_table[(color1 & tig_color_red_mask) >> tig_color_red_shift];

                                *(uint32_t*)dst_pixels = ((red2 + ((v1 * (red3 - red2)) >> 8)) & tig_color_red_mask)
                                    | ((green2 + ((v1 * (green3 - green2)) >> 8)) & tig_color_green_mask)
                                    | ((blue2 + ((v1 * (blue3 - blue2)) >> 8)) & tig_color_blue_mask);
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_GRADIENT_ANY) != 0) {
                // 0x50AF93
                switch (blt->flags & TIG_ART_BLT_BLEND_MODE_GRADIENT_ANY) {
                case TIG_ART_BLT_BLEND_MODE_GRADIENT_HORIZONTAL:
                    start_opacity_vertical_step = 0.0;
                    end_opacity_vertical_step = 0.0;
                    start_opacity_x = (float)src_rect.x;
                    end_opacity_x = (float)(art->hdr.frames_tbl[rotation][frame].width - src_rect.width - src_rect.x);
                    opacity_range = ((float)blt->opacity[1] - (float)blt->opacity[0]) / art->hdr.frames_tbl[rotation][frame].width;
                    start_opacity = (float)blt->opacity[0] + start_opacity_x * opacity_range;
                    end_opacity = (float)blt->opacity[1] - end_opacity_x * opacity_range;
                    break;
                case TIG_ART_BLT_BLEND_MODE_GRADIENT_VERTICAL:
                    start_opacity_vertical_step = ((float)blt->opacity[3] - (float)blt->opacity[0]) / art->hdr.frames_tbl[rotation][frame].height;
                    end_opacity_vertical_step = start_opacity_vertical_step;
                    start_opacity = src_rect.y * start_opacity_vertical_step + (float)blt->opacity[0];
                    end_opacity = start_opacity;
                    break;
                default:
                    start_opacity_vertical_step = ((float)blt->opacity[3] - (float)blt->opacity[0]) / art->hdr.frames_tbl[rotation][frame].height;
                    end_opacity_vertical_step = ((float)(uint8_t)blt->opacity[2] - (float)blt->opacity[1]) / art->hdr.frames_tbl[rotation][frame].height;
                    start_opacity_x = (float)src_rect.x;
                    end_opacity_x = (float)(art->hdr.frames_tbl[rotation][frame].width - src_rect.width - src_rect.x);
                    opacity_range = ((src_rect.y * end_opacity_vertical_step + (float)blt->opacity[1]) - (src_rect.y * start_opacity_vertical_step + (float)blt->opacity[0])) / art->hdr.frames_tbl[rotation][frame].width;
                    start_opacity = (src_rect.y * start_opacity_vertical_step + (float)blt->opacity[0]) + start_opacity_x * opacity_range;
                    end_opacity = (src_rect.y * end_opacity_vertical_step + (float)blt->opacity[1]) - end_opacity_x * opacity_range;
                    break;
                }

                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        current_opacity = start_opacity;
                        opacity_horizontal_step = (end_opacity - start_opacity) / src_rect.width;

                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t dst_color = *(uint32_t*)dst_pixels;
                                uint32_t src_color = tig_color_mult(((uint32_t*)plt)[*src_pixels], blt->field_10);

                                uint32_t dst_rm = tig_color_red_mask & dst_color;
                                uint32_t dst_gm = tig_color_green_mask & dst_color;
                                uint32_t dst_bm = tig_color_blue_mask & dst_color;

                                uint32_t src_rm = tig_color_red_mask & src_color;
                                uint32_t src_gm = tig_color_green_mask & src_color;
                                uint32_t src_bm = tig_color_blue_mask & src_color;

                                *(uint32_t*)dst_pixels = ((dst_rm + (((uint8_t)current_opacity * (src_rm - dst_rm)) >> 8)) & tig_color_red_mask)
                                    | ((dst_gm + (((uint8_t)current_opacity * (src_gm - dst_gm)) >> 8)) & tig_color_green_mask)
                                    | ((dst_bm + (((uint8_t)current_opacity * (src_bm - dst_bm)) >> 8)) & tig_color_blue_mask);
                            }

                            current_opacity += opacity_horizontal_step;

                            src_pixels += delta;
                            dst_pixels += 4;
                        }

                        start_opacity += start_opacity_vertical_step;
                        end_opacity += end_opacity_vertical_step;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_CHECKERBOARD_SRC) != 0) {
                // 0x50A2A6
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (((src_checkerboard_cur_x ^ src_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = tig_color_mult(*((uint32_t*)plt + *src_pixels), blt->field_10);
                                }
                            }
                            src_pixels += delta;
                            dst_pixels += 4;

                            src_checkerboard_cur_x++;
                        }
                        dst_pixels += dst_skip;

                        src_checkerboard_cur_x = src_rect.x;
                        src_checkerboard_cur_y++;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_CHECKERBOARD_DST) != 0) {
                // 0x50A739
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (((dst_checkerboard_cur_x ^ dst_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = tig_color_mult(*((uint32_t*)plt + *src_pixels), blt->field_10);
                                }
                            }
                            src_pixels += delta;
                            dst_pixels += 4;

                            dst_checkerboard_cur_x++;
                        }
                        dst_pixels += dst_skip;

                        dst_checkerboard_cur_x = dst_rect.x;
                        dst_checkerboard_cur_y++;
                    }
                    break;
                }
            } else {
                // 0x509647
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_mult(*((uint32_t*)plt + *src_pixels), blt->field_10);
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            }
        } else if ((blt->flags & TIG_ART_BLT_0x4000) != 0) {
            // 0x50BAEF
            if ((blt->flags & TIG_ART_BLT_0x10) != 0) {
                // 0x50BAFB
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        mask = &(blt->field_14[src_rect.x]);

                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mult(*((uint32_t*)plt + *src_pixels), *mask);
                                *(uint32_t*)dst_pixels = tig_color_plus(*(uint32_t*)dst_pixels, color);
                            }

                            mask++;
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x20) != 0) {
                // 0x50C06F
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        mask = &(blt->field_14[src_rect.x]);

                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color1 = *((uint32_t*)plt + *src_pixels);
                                uint32_t color2 = *(uint32_t*)dst_pixels;
                                uint32_t color3 = *mask;

                                unsigned int red2 = (color2 & tig_color_red_mask);
                                unsigned int green2 = (color2 & tig_color_green_mask);
                                unsigned int blue2 = (color2 & tig_color_blue_mask);
                                unsigned int red3 = (color3 & tig_color_red_mask);
                                unsigned int green3 = (color3 & tig_color_green_mask);
                                unsigned int blue3 = (color3 & tig_color_blue_mask);
                                unsigned int v1 = tig_color_red_platform_to_rgb_table[(color1 & tig_color_red_mask) >> tig_color_red_shift];

                                *(uint32_t*)dst_pixels = ((red2 + ((v1 * (red3 - red2)) >> 8)) & tig_color_red_mask)
                                    | ((green2 + ((v1 * (green3 - green2)) >> 8)) & tig_color_green_mask)
                                    | ((blue2 + ((v1 * (blue3 - blue2)) >> 8)) & tig_color_blue_mask);
                            }

                            mask++;
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x40) != 0) {
                // 0x50C646
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        mask = &(blt->field_14[src_rect.x]);

                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mult(*((uint32_t*)plt + *src_pixels), *mask);
                                *(uint32_t*)dst_pixels = tig_color_mult(color, *(uint32_t*)dst_pixels);
                            }

                            mask++;
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x80) != 0) {
                // 0x50CD23
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        mask = &(blt->field_14[src_rect.x]);

                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mult(*((uint32_t*)plt + *src_pixels), *mask);
                                *(uint32_t*)dst_pixels = tig_color_blend(color,
                                    *(uint32_t*)dst_pixels,
                                    tig_color_rgb_to_grayscale(color));
                            }

                            mask++;
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x100) != 0) {
                // 0x50D3F9
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        mask = &(blt->field_14[src_rect.x]);

                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mult(*((uint32_t*)plt + *src_pixels), *mask);
                                *(uint32_t*)dst_pixels = tig_color_blend(color, *(uint32_t*)dst_pixels, blt->opacity[0]);
                            }

                            mask++;
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x200) != 0) {
                // 0x50E3C4
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        mask = &(blt->field_14[src_rect.x]);

                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color1 = *((uint32_t*)plt + *src_pixels);
                                uint32_t color2 = *(uint32_t*)dst_pixels;
                                uint32_t color3 = blt->field_10;

                                unsigned int red2 = (color2 & tig_color_red_mask);
                                unsigned int green2 = (color2 & tig_color_green_mask);
                                unsigned int blue2 = (color2 & tig_color_blue_mask);
                                unsigned int red3 = (color3 & tig_color_red_mask);
                                unsigned int green3 = (color3 & tig_color_green_mask);
                                unsigned int blue3 = (color3 & tig_color_blue_mask);
                                unsigned int v1 = tig_color_red_platform_to_rgb_table[(color1 & tig_color_red_mask) >> tig_color_red_shift];

                                *(uint32_t*)dst_pixels = ((red2 + ((v1 * (red3 - red2)) >> 8)) & tig_color_red_mask)
                                    | ((green2 + ((v1 * (green3 - green2)) >> 8)) & tig_color_green_mask)
                                    | ((blue2 + ((v1 * (blue3 - blue2)) >> 8)) & tig_color_blue_mask);
                            }

                            mask++;
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_GRADIENT_ANY) != 0) {
                // 0x50E7CB
                switch (blt->flags & TIG_ART_BLT_BLEND_MODE_GRADIENT_ANY) {
                case TIG_ART_BLT_BLEND_MODE_GRADIENT_HORIZONTAL:
                    start_opacity_vertical_step = 0.0;
                    end_opacity_vertical_step = 0.0;
                    start_opacity_x = (float)src_rect.x;
                    end_opacity_x = (float)(art->hdr.frames_tbl[rotation][frame].width - src_rect.width - src_rect.x);
                    opacity_range = ((float)blt->opacity[1] - (float)blt->opacity[0]) / art->hdr.frames_tbl[rotation][frame].width;
                    start_opacity = (float)blt->opacity[0] + start_opacity_x * opacity_range;
                    end_opacity = (float)blt->opacity[1] - end_opacity_x * opacity_range;
                    break;
                case TIG_ART_BLT_BLEND_MODE_GRADIENT_VERTICAL:
                    start_opacity_vertical_step = ((float)blt->opacity[3] - (float)blt->opacity[0]) / art->hdr.frames_tbl[rotation][frame].height;
                    end_opacity_vertical_step = start_opacity_vertical_step;
                    start_opacity = src_rect.y * start_opacity_vertical_step + (float)blt->opacity[0];
                    end_opacity = start_opacity;
                    break;
                default:
                    start_opacity_vertical_step = ((float)blt->opacity[3] - (float)blt->opacity[0]) / art->hdr.frames_tbl[rotation][frame].height;
                    end_opacity_vertical_step = ((float)(uint8_t)blt->opacity[2] - (float)blt->opacity[1]) / art->hdr.frames_tbl[rotation][frame].height;
                    start_opacity_x = (float)src_rect.x;
                    end_opacity_x = (float)(art->hdr.frames_tbl[rotation][frame].width - src_rect.width - src_rect.x);
                    opacity_range = ((src_rect.y * end_opacity_vertical_step + (float)blt->opacity[1]) - (src_rect.y * start_opacity_vertical_step + (float)blt->opacity[0])) / art->hdr.frames_tbl[rotation][frame].width;
                    start_opacity = (src_rect.y * start_opacity_vertical_step + (float)blt->opacity[0]) + start_opacity_x * opacity_range;
                    end_opacity = (src_rect.y * end_opacity_vertical_step + (float)blt->opacity[1]) - end_opacity_x * opacity_range;
                    break;
                }

                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        current_opacity = start_opacity;
                        opacity_horizontal_step = (end_opacity - start_opacity) / src_rect.width;
                        mask = &(blt->field_14[src_rect.x]);

                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t dst_color = *(uint32_t*)dst_pixels;
                                uint32_t src_color = tig_color_mult(((uint32_t*)plt)[*src_pixels], *mask);

                                uint32_t dst_rm = tig_color_red_mask & dst_color;
                                uint32_t dst_gm = tig_color_green_mask & dst_color;
                                uint32_t dst_bm = tig_color_blue_mask & dst_color;

                                uint32_t src_rm = tig_color_red_mask & src_color;
                                uint32_t src_gm = tig_color_green_mask & src_color;
                                uint32_t src_bm = tig_color_blue_mask & src_color;

                                *(uint32_t*)dst_pixels = ((dst_rm + (((uint8_t)current_opacity * (src_rm - dst_rm)) >> 8)) & tig_color_red_mask)
                                    | ((dst_gm + (((uint8_t)current_opacity * (src_gm - dst_gm)) >> 8)) & tig_color_green_mask)
                                    | ((dst_bm + (((uint8_t)current_opacity * (src_bm - dst_bm)) >> 8)) & tig_color_blue_mask);
                            }

                            current_opacity += opacity_horizontal_step;

                            mask++;
                            src_pixels += delta;
                            dst_pixels += 4;
                        }

                        start_opacity += start_opacity_vertical_step;
                        end_opacity += end_opacity_vertical_step;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_CHECKERBOARD_SRC) != 0) {
                // 0x50DA0F
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        mask = &(blt->field_14[src_rect.x]);

                        for (x = 0; x < width; x++) {
                            if (((src_checkerboard_cur_x ^ src_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = tig_color_mult(*((uint32_t*)plt + *src_pixels), *mask);
                                }
                            }

                            mask++;
                            src_pixels += delta;
                            dst_pixels += 4;

                            src_checkerboard_cur_x++;
                        }
                        dst_pixels += dst_skip;

                        src_checkerboard_cur_x = src_rect.x;
                        src_checkerboard_cur_y++;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_CHECKERBOARD_DST) != 0) {
                // 0x50DEE7
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        mask = &(blt->field_14[src_rect.x]);

                        for (x = 0; x < width; x++) {
                            if (((dst_checkerboard_cur_x ^ dst_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = tig_color_mult(*((uint32_t*)plt + *src_pixels), *mask);
                                }
                            }

                            mask++;
                            src_pixels += delta;
                            dst_pixels += 4;

                            dst_checkerboard_cur_x++;
                        }
                        dst_pixels += dst_skip;

                        dst_checkerboard_cur_x = dst_rect.x;
                        dst_checkerboard_cur_y++;
                    }
                    break;
                }
            } else {
                // 0x50CD1B
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        mask = &(blt->field_14[src_rect.x]);

                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_mult(*((uint32_t*)plt + *src_pixels), *mask);
                            }

                            mask++;
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            }
        } else if ((blt->flags & TIG_ART_BLT_0x20000) != 0) {
            // 0x50F3B4
        } else {
            // 0x50655B
            if ((blt->flags & TIG_ART_BLT_0x10) != 0) {
                // 0x506566
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_plus(*((uint32_t*)plt + *src_pixels), *(uint32_t*)dst_pixels);
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x20) != 0) {
                // 0x506838
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                if ((*(uint32_t*)dst_pixels & tig_color_red_mask) < ((*((uint32_t*)plt + *src_pixels)) & tig_color_red_mask)) {
                                    *(uint32_t*)dst_pixels = (*(uint32_t*)dst_pixels & ~tig_color_red_mask) | ((*((uint32_t*)plt + *src_pixels)) & tig_color_red_mask);
                                }
                                if ((*(uint32_t*)dst_pixels & tig_color_green_mask) < ((*((uint32_t*)plt + *src_pixels)) & tig_color_green_mask)) {
                                    *(uint32_t*)dst_pixels = (*(uint32_t*)dst_pixels & ~tig_color_green_mask) | ((*((uint32_t*)plt + *src_pixels)) & tig_color_green_mask);
                                }
                                if ((*(uint32_t*)dst_pixels & tig_color_blue_mask) < ((*((uint32_t*)plt + *src_pixels)) & tig_color_blue_mask)) {
                                    *(uint32_t*)dst_pixels = (*(uint32_t*)dst_pixels & ~tig_color_blue_mask) | ((*((uint32_t*)plt + *src_pixels)) & tig_color_blue_mask);
                                }
                                *(uint32_t*)dst_pixels -= *((uint32_t*)plt + *src_pixels);
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x40) != 0) {
                // 0x506B87
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_mult(*((uint32_t*)plt + *src_pixels), *(uint32_t*)dst_pixels);
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x80) != 0) {
                // 0x506FD0
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend(*((uint32_t*)plt + *src_pixels),
                                    *(uint32_t*)dst_pixels,
                                    tig_color_rgb_to_grayscale(*((uint32_t*)plt + *src_pixels)));
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x100) != 0) {
                // 0x50742A
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend(*((uint32_t*)plt + *src_pixels), *(uint32_t*)dst_pixels, blt->opacity[0]);
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_0x200) != 0) {
                // 0x507B0F
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_red_opacity(*((uint32_t*)plt + *src_pixels), *(uint32_t*)dst_pixels);
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_GRADIENT_ANY) != 0) {
                // 0x507ECB
                switch (blt->flags & TIG_ART_BLT_BLEND_MODE_GRADIENT_ANY) {
                case TIG_ART_BLT_BLEND_MODE_GRADIENT_HORIZONTAL:
                    start_opacity_vertical_step = 0.0;
                    end_opacity_vertical_step = 0.0;
                    start_opacity_x = (float)src_rect.x;
                    end_opacity_x = (float)(art->hdr.frames_tbl[rotation][frame].width - src_rect.width - src_rect.x);
                    opacity_range = ((float)blt->opacity[1] - (float)blt->opacity[0]) / art->hdr.frames_tbl[rotation][frame].width;
                    start_opacity = (float)blt->opacity[0] + start_opacity_x * opacity_range;
                    end_opacity = (float)blt->opacity[1] - end_opacity_x * opacity_range;
                    break;
                case TIG_ART_BLT_BLEND_MODE_GRADIENT_VERTICAL:
                    start_opacity_vertical_step = ((float)blt->opacity[3] - (float)blt->opacity[0]) / art->hdr.frames_tbl[rotation][frame].height;
                    end_opacity_vertical_step = start_opacity_vertical_step;
                    start_opacity = src_rect.y * start_opacity_vertical_step + (float)blt->opacity[0];
                    end_opacity = start_opacity;
                    break;
                default:
                    start_opacity_vertical_step = ((float)blt->opacity[3] - (float)blt->opacity[0]) / art->hdr.frames_tbl[rotation][frame].height;
                    end_opacity_vertical_step = ((float)(uint8_t)blt->opacity[2] - (float)blt->opacity[1]) / art->hdr.frames_tbl[rotation][frame].height;
                    start_opacity_x = (float)src_rect.x;
                    end_opacity_x = (float)(art->hdr.frames_tbl[rotation][frame].width - src_rect.width - src_rect.x);
                    opacity_range = ((src_rect.y * end_opacity_vertical_step + (float)blt->opacity[1]) - (src_rect.y * start_opacity_vertical_step + (float)blt->opacity[0])) / art->hdr.frames_tbl[rotation][frame].width;
                    start_opacity = (src_rect.y * start_opacity_vertical_step + (float)blt->opacity[0]) + start_opacity_x * opacity_range;
                    end_opacity = (src_rect.y * end_opacity_vertical_step + (float)blt->opacity[1]) - end_opacity_x * opacity_range;
                    break;
                }

                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        current_opacity = start_opacity;
                        opacity_horizontal_step = (end_opacity - start_opacity) / src_rect.width;

                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t dst_color = *(uint32_t*)dst_pixels;
                                uint32_t src_color = ((uint32_t*)plt)[*src_pixels];

                                uint32_t dst_rm = tig_color_red_mask & dst_color;
                                uint32_t dst_gm = tig_color_green_mask & dst_color;
                                uint32_t dst_bm = tig_color_blue_mask & dst_color;

                                uint32_t src_rm = tig_color_red_mask & src_color;
                                uint32_t src_gm = tig_color_green_mask & src_color;
                                uint32_t src_bm = tig_color_blue_mask & src_color;

                                *(uint32_t*)dst_pixels = ((dst_rm + (((uint8_t)current_opacity * (src_rm - dst_rm)) >> 8)) & tig_color_red_mask)
                                    | ((dst_gm + (((uint8_t)current_opacity * (src_gm - dst_gm)) >> 8)) & tig_color_green_mask)
                                    | ((dst_bm + (((uint8_t)current_opacity * (src_bm - dst_bm)) >> 8)) & tig_color_blue_mask);
                            }

                            current_opacity += opacity_horizontal_step;

                            src_pixels += delta;
                            dst_pixels += 4;
                        }

                        start_opacity += start_opacity_vertical_step;
                        end_opacity += end_opacity_vertical_step;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_CHECKERBOARD_SRC) != 0) {
                // 0x50779F
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (((src_checkerboard_cur_x ^ src_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = *((uint32_t*)plt + *src_pixels);
                                }
                            }
                            src_pixels += delta;
                            dst_pixels += 4;

                            src_checkerboard_cur_x++;
                        }
                        dst_pixels += dst_skip;

                        src_checkerboard_cur_x = src_rect.x;
                        src_checkerboard_cur_y++;
                    }
                    break;
                }
            } else if ((blt->flags & TIG_ART_BLT_BLEND_MODE_CHECKERBOARD_DST) != 0) {
                // 0x507955
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (((dst_checkerboard_cur_x ^ dst_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = *((uint32_t*)plt + *src_pixels);
                                }
                            }
                            src_pixels += delta;
                            dst_pixels += 4;

                            dst_checkerboard_cur_x++;
                        }
                        dst_pixels += dst_skip;

                        dst_checkerboard_cur_x = dst_rect.x;
                        dst_checkerboard_cur_y++;
                    }
                    break;
                }
            } else {
                // 0x506FC8
                switch (tig_art_bits_per_pixel) {
                case 32:
                    if (delta > 0) {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
                        dst_skip = video_buffer_data.pitch - width * 4;
                    } else {
                        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
                        dst_skip = -width * 4 - video_buffer_data.pitch;
                    }
                    dst_pixels += video_buffer_data.pitch * blt->dst_rect->y + blt->dst_rect->x * 4;
                    for (y = 0; y < height; y++) {
                        for (x = 0; x < width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = *((uint32_t*)plt + *src_pixels);
                            }
                            src_pixels += delta;
                            dst_pixels += 4;
                        }
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            }
        }
    }

    tig_video_buffer_unlock(blt->dst_video_buffer);

    return TIG_OK;
}

// 0x51AA90
unsigned int sub_51AA90(tig_art_id_t art_id)
{
    char path[_MAX_PATH];
    int cache_entry_index;

    if (tig_art_build_path(art_id, path) != TIG_OK) {
        return (unsigned int)-1;
    }

    if (dword_604714 < tig_art_cache_entries_length
        && strcmp(path, tig_art_cache_entries[dword_604714].path) == 0) {
        tig_art_cache_entries[dword_604714].time = tig_ping_time;
        return dword_604714;
    }

    // Called twice to check both system and video memory.
    tig_art_cache_check_fullness();
    tig_art_cache_check_fullness();

    if (!tig_art_cache_find(path, &cache_entry_index)) {
        if (!tig_art_cache_entry_load(art_id, path, cache_entry_index)) {
            tig_debug_printf("ART LOAD FAILURE!!! Trying to load %s\n", path);

            if (!tig_art_cache_entry_load(art_id, "art\\badart.art", cache_entry_index)) {
                tig_debug_printf("ART LOAD FAILURE!!! Trying to load badart.art\n");
                return (unsigned int)-1;
            }
        }
    }

    tig_art_cache_entries[cache_entry_index].time = tig_ping_time;
    tig_art_cache_entries[cache_entry_index].art_id = art_id;
    dword_604714 = cache_entry_index;

    return cache_entry_index;
}

// 0x51AC00
void tig_art_cache_set_video_memory_fullness(int fullness)
{
    tig_art_cache_video_memory_fullness = (float)fullness / 100.0f;
}

// 0x51AC20
void tig_art_cache_check_fullness()
{
    // Flag that denotes if we should check video memory (true) or system memory
    // (false). This flag alternates on every call.
    //
    // 0x604728
    static bool vid_vs_sys;

    art_size_t acc = 0;
    art_size_t tgt;
    unsigned int index;
    unsigned int cnt;

    if (vid_vs_sys) {
        // NOTE: Signed compare.
        if (tig_art_available_video_memory > 0) {
            vid_vs_sys = !vid_vs_sys;
            return;
        }

        tig_debug_printf("Art cache full (vid), making some room");

        // Calculate target size we'd like to evict.
        tgt = (art_size_t)((double)tig_art_total_video_memory * tig_art_cache_video_memory_fullness);

        // Sort cache entries by last access time, earliest first.
        qsort(tig_art_cache_entries,
            tig_art_cache_entries_length,
            sizeof(TigArtCacheEntry),
            tig_art_cache_entry_compare_time);

        // Loop thru cache entries until we reach eviction target.
        for (index = 0; index < tig_art_cache_entries_length; ++index) {
            acc += tig_art_cache_entries[index].video_memory_usage;
            if (acc >= tgt) {
                break;
            }
        }
    } else {
        // NOTE: Signed compare.
        if (tig_art_available_system_memory > 0) {
            vid_vs_sys = !vid_vs_sys;
            return;
        }

        tig_debug_printf("Art cache full (sys), making some room");

        // Calculate target size we'd like to evict (30% of total system
        // memory).
        tgt = (size_t)((double)tig_art_total_system_memory * 0.3f);

        // Sort cache entries by last access time, earliest first.
        qsort(tig_art_cache_entries,
            tig_art_cache_entries_length,
            sizeof(TigArtCacheEntry),
            tig_art_cache_entry_compare_time);

        // Loop thru cache entries until we reach eviction target.
        for (index = 0; index < tig_art_cache_entries_length; ++index) {
            acc += tig_art_cache_entries[index].system_memory_usage;
            if (acc >= tgt) {
                break;
            }
        }
    }

    // NOTE: Signed compare.
    if (acc < tgt) {
        // We haven't reached eviction target, flush everything.
        tig_art_flush();
        tig_debug_printf("...\n");
        return;
    }

    cnt = index + 1;

    // Free cache entries.
    for (; index >= 0; index--) {
        tig_art_cache_entry_unload(index);
    }

    tig_art_cache_entries_length -= cnt;

    // Move remaining cache entries on top.
    memcpy(tig_art_cache_entries,
        &(tig_art_cache_entries[cnt]),
        sizeof(TigArtCacheEntry) * (tig_art_cache_entries_length));

    // Sort back cache entries by name (as `tig_art_cache_find` uses binary
    // search by name).
    qsort(tig_art_cache_entries,
        tig_art_cache_entries_length,
        sizeof(TigArtCacheEntry),
        tig_art_cache_entry_compare_name);

    tig_debug_printf("...\n");
    vid_vs_sys = !vid_vs_sys;
}

// 0x51ADE0
int tig_art_cache_entry_compare_time(const void* a1, const void* a2)
{
    TigArtCacheEntry* a = (TigArtCacheEntry*)a1;
    TigArtCacheEntry* b = (TigArtCacheEntry*)a2;
    return a->time - b->time;
}

// 0x51AE00
int tig_art_cache_entry_compare_name(const void* a1, const void* a2)
{
    TigArtCacheEntry* a = (TigArtCacheEntry*)a1;
    TigArtCacheEntry* b = (TigArtCacheEntry*)a2;
    return strcmp(a->path, b->path);
}

// 0x51AE50
int tig_art_build_path(unsigned int art_id, char* path)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_MISC) {
        switch (tig_art_num(art_id)) {
        case TIG_ART_SYSTEM_MOUSE:
            strcpy(path, "art\\mouse.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_BUTTON:
            strcpy(path, "art\\button.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_UP:
            strcpy(path, "art\\up.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_DOWN:
            strcpy(path, "art\\down.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_CANCEL:
            strcpy(path, "art\\cancel.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_LENS:
            strcpy(path, "art\\lens.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_X:
            strcpy(path, "art\\x.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_PLUS:
            strcpy(path, "art\\plus.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_MINUS:
            strcpy(path, "art\\minus.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_BLANK:
            strcpy(path, "art\\blank.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_FONT:
            strcpy(path, "art\\morph15font.art");
            return TIG_OK;
        }
    } else {
        if (tig_art_file_path_resolver != NULL) {
            return tig_art_file_path_resolver(art_id, path);
        }
    }

    return TIG_ERR_16;
}

// 0x51B0A0
tig_art_id_t sub_51B0A0(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_MISC) {
        return tig_art_id_frame_set(tig_art_set_palette(art_id, 0), 0);
    }

    if (dword_604750 != NULL) {
        return dword_604750(art_id);
    }

    return art_id;
}

// TODO: Check.
//
// 0x51B0E0
bool tig_art_cache_find(const char* path, int* index)
{
    int l = 0;
    int r = tig_art_cache_entries_length - 1;

    while (l <= r) {
        int mid = (l + r) / 2;
        int cmp = strcmp(tig_art_cache_entries[mid].path, path);
        if (cmp == 0) {
            *index = mid;
            return true;
        }

        if (cmp > 0) {
            r = mid - 1;
        } else {
            l = mid + 1;
        }
    }

    *index = 0;
    return false;
}

// 0x51B170
bool tig_art_cache_entry_load(tig_art_id_t art_id, const char* path, int cache_entry_index)
{
    TigArtCacheEntry* art;
    int rc;
    art_size_t size;
    int type;
    int start;
    int num_rotations;
    int rotation;
    int index;
    int frame;
    int offset;

    if (tig_art_cache_entries_length == tig_art_cache_entries_capacity - 1) {
        tig_art_cache_entries_capacity += 32;
        tig_art_cache_entries = (TigArtCacheEntry*)REALLOC(tig_art_cache_entries,
            sizeof(TigArtCacheEntry) * tig_art_cache_entries_capacity);
    }

    memcpy(&(tig_art_cache_entries[cache_entry_index + 1]),
        &(tig_art_cache_entries[cache_entry_index]),
        sizeof(TigArtCacheEntry) * (tig_art_cache_entries_length - cache_entry_index));

    art = &(tig_art_cache_entries[cache_entry_index]);

    memset(art, 0, sizeof(TigArtCacheEntry));
    strcpy(art->path, path);

    rc = sub_51B710(art_id,
        path,
        &(art->hdr),
        art->field_254,
        0,
        &size);
    if (rc != TIG_OK) {
        memcpy(&(tig_art_cache_entries[cache_entry_index]),
            &(tig_art_cache_entries[cache_entry_index + 1]),
            sizeof(TigArtCacheEntry) * (tig_art_cache_entries_length - cache_entry_index));
        return false;
    }

    art->system_memory_usage += size;

    type = tig_art_type(art_id);
    if ((art->hdr.flags & TIG_ART_0x01) != 0) {
        start = 0;
        num_rotations = 1;
    } else if (tig_art_mirroring_enabled
        && (type == TIG_ART_TYPE_CRITTER
            || type == TIG_ART_TYPE_MONSTER
            || type == TIG_ART_TYPE_UNIQUE_NPC)) {
        start = 4;
        num_rotations = 5;
    } else {
        start = 0;
        num_rotations = MAX_ROTATIONS;
    }

    for (index = 0; index < num_rotations; index++) {
        rotation = (index + start) % MAX_ROTATIONS;
        art->pixels_tbl[rotation] = (uint8_t**)MALLOC(sizeof(uint8_t*) * art->hdr.num_frames);
        art->system_memory_usage += sizeof(uint8_t*) * art->hdr.num_frames;

        offset = 0;
        for (frame = 0; frame < art->hdr.num_frames; ++frame) {
            art->pixels_tbl[rotation][frame] = art->hdr.pixels_tbl[rotation] + offset;
            offset += art->hdr.frames_tbl[rotation][frame].width * art->hdr.frames_tbl[rotation][frame].height;
        }
    }

    for (index = MAX_ROTATIONS - num_rotations; index > 0; --index) {
        rotation = (num_rotations + start) % MAX_ROTATIONS;
        art->pixels_tbl[rotation] = art->pixels_tbl[0];
    }

    tig_art_cache_entries_length++;
    tig_art_available_system_memory -= art->system_memory_usage;

    return true;
}

// 0x51B490
void tig_art_cache_entry_unload(unsigned int cache_entry_index)
{
    TigArtCacheEntry* cache_entry;
    int type;
    int rotation_start;
    int num_rotations;
    int rotation;
    int palette;

    cache_entry = &(tig_art_cache_entries[cache_entry_index]);

    tig_art_available_system_memory += cache_entry->system_memory_usage;
    tig_art_available_video_memory += cache_entry->video_memory_usage;

    sub_51B650(cache_entry_index);

    type = tig_art_type(cache_entry->art_id);

    if ((cache_entry->hdr.flags & TIG_ART_0x01) != 0) {
        rotation_start = 0;
        num_rotations = 1;
    } else if (tig_art_mirroring_enabled
        && (type == TIG_ART_TYPE_CRITTER
            || type == TIG_ART_TYPE_MONSTER
            || type == TIG_ART_TYPE_UNIQUE_NPC)) {
        rotation_start = 4;
        num_rotations = 5;
    } else {
        rotation_start = 0;
        num_rotations = MAX_ROTATIONS;
    }

    for (rotation = 0; rotation < num_rotations; ++rotation) {
        FREE(cache_entry->pixels_tbl[rotation]);
        FREE(cache_entry->hdr.pixels_tbl[rotation]);
        FREE(cache_entry->hdr.frames_tbl[rotation]);
    }

    for (palette = 0; palette < MAX_PALETTES; ++palette) {
        if (cache_entry->hdr.palette_tbl[palette] != NULL) {
            tig_palette_destroy(cache_entry->hdr.palette_tbl[palette]);
        }

        if (cache_entry->field_254[palette] != NULL) {
            tig_palette_destroy(cache_entry->field_254[palette]);
        }
    }
}

// 0x51B610
void sub_51B610(unsigned int cache_entry_index)
{
    int palette;
    int rotation;

    for (palette = 0; palette < MAX_PALETTES; palette++) {
        for (rotation = 0; rotation < MAX_ROTATIONS; rotation++) {
            tig_art_cache_entries[cache_entry_index].field_194[palette][rotation] = 1;
        }
    }
}

// 0x51B650
void sub_51B650(int cache_entry_index)
{
    TigArtCacheEntry* art;
    int num_frames;
    int palette;
    int rotation;
    int frame;

    art = &(tig_art_cache_entries[cache_entry_index]);

    if (tig_art_type(art->art_id) == TIG_ART_TYPE_ROOF) {
        num_frames = 13;
    } else {
        num_frames = art->hdr.num_frames;
    }

    for (palette = 0; palette < MAX_PALETTES; palette++) {
        for (rotation = 0; rotation < MAX_ROTATIONS; rotation++) {
            if (art->video_buffers[palette][rotation] != NULL) {
                for (frame = 0; frame < num_frames; frame++) {
                    tig_video_buffer_destroy(art->video_buffers[palette][rotation][frame]);
                }

                FREE(art->video_buffers[palette][rotation]);
                art->video_buffers[palette][rotation] = NULL;
            }
        }
    }
}

// 0x51B710
int sub_51B710(tig_art_id_t art_id, const char* filename, TigArtHeader* hdr, void** palette_tbl, int a5, art_size_t* size_ptr)
{
    TigFile* stream;
    int rotation;
    int palette;
    uint32_t* saved_palette_tbl[MAX_PALETTES];
    uint32_t temp_palette_entries[256];
    art_size_t size_tbl[MAX_ROTATIONS];
    int index;
    int frame;
    int current_palette_index;
    void* current_palette;
    int num_rotations;

    // NOTE: Keep compiler happy.
    current_palette_index = 0;
    current_palette = NULL;

    *size_ptr = 0;

    for (rotation = 0; rotation < MAX_ROTATIONS; rotation++) {
        hdr->frames_tbl[rotation] = NULL;
        hdr->pixels_tbl[rotation] = NULL;
    }

    stream = tig_file_fopen(filename, "rb");
    if (stream == NULL) {
        return TIG_ERR_16;
    }

    if (tig_file_fread(hdr, sizeof(TigArtHeader), 1, stream) != 1) {
        tig_file_fclose(stream);
        return TIG_ERR_16;
    }

    // Only 8-bpp palette-indexed ART files are supported.
    if (hdr->bpp != 8) {
        tig_file_fclose(stream);
        return TIG_ERR_16;
    }

    if (a5) {
        current_palette_index = tig_art_palette(art_id);
        if (hdr->palette_tbl[current_palette_index] != NULL) {
            tig_file_fclose(stream);
            return TIG_ERR_16;
        }
        current_palette = palette_tbl[0];
    }

    for (palette = 0; palette < MAX_PALETTES; palette++) {
        saved_palette_tbl[palette] = hdr->palette_tbl[palette];
        hdr->palette_tbl[palette] = NULL;
        palette_tbl[palette] = NULL;
    }

    for (palette = 0; palette < MAX_PALETTES; palette++) {
        if (saved_palette_tbl[palette] != NULL) {
            if (tig_file_fread(temp_palette_entries, sizeof(uint32_t), 256, stream) != 256) {
                sub_51BE50(stream, hdr, palette_tbl);
                return TIG_ERR_16;
            }

            if (a5) {
                if (palette == current_palette_index) {
                    for (index = 0; index < 256; index++) {
                        ((uint32_t*)current_palette)[index] = temp_palette_entries[index];
                    }

                    tig_file_fclose(stream);
                    return TIG_OK;
                }
            } else {
                hdr->palette_tbl[palette] = tig_palette_create();
                palette_tbl[palette] = tig_palette_create();
                *size_ptr += 2 * tig_palette_get_size();

                switch (tig_art_bits_per_pixel) {
                case 8:
                    for (index = 0; index < 256; index++) {
                        ((uint8_t*)hdr->palette_tbl[palette])[index] = (uint8_t)tig_color_index_of(temp_palette_entries[index]);
                    }
                    break;
                case 16:
                    for (index = 0; index < 256; index++) {
                        ((uint16_t*)hdr->palette_tbl[palette])[index] = (uint16_t)tig_color_index_of(temp_palette_entries[index]);
                    }
                    break;
                case 24:
                    for (index = 0; index < 256; index++) {
                        ((uint32_t*)hdr->palette_tbl[palette])[index] = (uint32_t)tig_color_index_of(temp_palette_entries[index]);
                    }
                    break;
                case 32:
                    for (index = 0; index < 256; index++) {
                        ((uint32_t*)hdr->palette_tbl[palette])[index] = (uint32_t)tig_color_index_of(temp_palette_entries[index]);
                    }
                    break;
                }
            }

            sub_505000(art_id, hdr->palette_tbl[palette], palette_tbl[palette]);
        }
    }

    num_rotations = sub_51BE30(hdr);

    for (rotation = 0; rotation < num_rotations; rotation++) {
        hdr->frames_tbl[rotation] = (TigArtFileFrameData*)MALLOC(sizeof(TigArtFileFrameData) * hdr->num_frames);
        *size_ptr += sizeof(TigArtFileFrameData) * hdr->num_frames;

        if (tig_file_fread(hdr->frames_tbl[rotation], sizeof(TigArtFileFrameData), hdr->num_frames, stream) != hdr->num_frames) {
            sub_51BE50(stream, hdr, palette_tbl);
            return TIG_ERR_16;
        }
    }

    for (index = 0; index < num_rotations; index++) {
        uint8_t* bytes;
        art_size_t total_size;

        // Calculate total size of pixels data.
        total_size = 0;
        for (frame = 0; frame < hdr->num_frames; ++frame) {
            total_size += hdr->frames_tbl[index][frame].width * hdr->frames_tbl[index][frame].height;
        }

        // Allocate appropriate memory.
        hdr->pixels_tbl[index] = (uint8_t*)MALLOC(total_size);

        // Store size for later use.
        size_tbl[index] = total_size;
        *size_ptr += total_size;

        // Read raw pixel data for each frame.
        bytes = hdr->pixels_tbl[index];
        for (frame = 0; frame < hdr->num_frames; ++frame) {
            if (hdr->frames_tbl[index][frame].data_size == hdr->frames_tbl[index][frame].width * hdr->frames_tbl[index][frame].height) {
                // Pixels are not compressed, read everything in one go.
                if (tig_file_fread(hdr->pixels_tbl[index], 1, hdr->frames_tbl[index][frame].data_size, stream) != hdr->frames_tbl[index][frame].data_size) {
                    sub_51BE50(stream, hdr, palette_tbl);
                    return TIG_ERR_16;
                }
            } else if (hdr->frames_tbl[index][frame].data_size > 0) {
                // Pixels are RLE-encoded.
                uint8_t value;
                uint8_t color;
                int len;
                int cnt = 0;

                while (cnt < hdr->frames_tbl[index][frame].data_size) {
                    if (tig_file_fread(&value, 1, 1, stream) != 1) {
                        sub_51BE50(stream, hdr, palette_tbl);
                        return TIG_ERR_16;
                    }

                    len = value & 0x7F;
                    if ((value & 0x80) != 0) {
                        if (tig_file_fread(bytes, 1, len, stream) != len) {
                            sub_51BE50(stream, hdr, palette_tbl);
                            return TIG_ERR_16;
                        }
                        cnt += 1 + len;
                    } else {
                        if (tig_file_fread(&color, 1, 1, stream) != 1) {
                            sub_51BE50(stream, hdr, palette_tbl);
                            return TIG_ERR_16;
                        }

                        memset(bytes, color, len);
                        cnt += 2;
                    }
                    bytes += len;
                }
            }
        }
    }

    while (index < MAX_ROTATIONS) {
        hdr->frames_tbl[index] = hdr->frames_tbl[0];
        hdr->pixels_tbl[index] = hdr->pixels_tbl[0];
        index++;
    }

    if (num_rotations > 1) {
        int type = tig_art_type(art_id);
        if (tig_art_mirroring_enabled) {
            if (type == TIG_ART_TYPE_CRITTER
                || type == TIG_ART_TYPE_MONSTER
                || type == TIG_ART_TYPE_UNIQUE_NPC) {
                for (index = 0; index < 3; index++) {
                    *size_ptr -= size_tbl[1 + index];

                    FREE(hdr->pixels_tbl[1 + index]);
                    hdr->pixels_tbl[1 + index] = hdr->pixels_tbl[0];

                    FREE(hdr->frames_tbl[1 + index]);
                    hdr->frames_tbl[1 + index] = hdr->frames_tbl[0];
                }
            }
        }
    }

    tig_file_fclose(stream);
    return TIG_OK;
}

// 0x51BE30
int sub_51BE30(TigArtHeader* hdr)
{
    return (hdr->flags & TIG_ART_0x01) != 0 ? 1 : MAX_ROTATIONS;
}

// 0x51BE50
void sub_51BE50(TigFile* stream, TigArtHeader* hdr, TigPalette* palette_tbl)
{
    int palette;

    if (stream != NULL) {
        tig_file_fclose(stream);
    }

    sub_51BF20(hdr);

    for (palette = 0; palette < MAX_PALETTES; ++palette) {
        if (hdr->palette_tbl[palette] != NULL) {
            tig_palette_destroy(hdr->palette_tbl[palette]);
        }

        if (palette_tbl[palette] != NULL) {
            tig_palette_destroy(palette_tbl[palette]);
        }
    }
}

// 0x51BEB0
void sub_51BEB0(FILE* stream, TigArtHeader* hdr, const char* filename, FILE* shd_stream, TigShdHeader* shd, const char* shd_filename)
{
    if (stream != NULL) {
        fclose(stream);
    }

    if (filename != NULL) {
        remove(filename);
    }

    if (shd_stream != NULL) {
        fclose(shd_stream);
    }

    if (shd_filename != NULL) {
        remove(shd_filename);
    }

    sub_51BF60(shd);
    sub_51BF20(hdr);
}

// 0x51BF20
void sub_51BF20(TigArtHeader* hdr)
{
    int num_rotations;
    int rotation;

    num_rotations = sub_51BE30(hdr);
    for (rotation = 0; rotation < num_rotations; rotation++) {
        if (hdr->pixels_tbl[rotation] != NULL) {
            FREE(hdr->pixels_tbl[rotation]);
        }
        if (hdr->frames_tbl[rotation] != NULL) {
            FREE(hdr->frames_tbl[rotation]);
        }
    }
}

// 0x51BF60
void sub_51BF60(TigShdHeader* shd)
{
    int num_rotations;
    int rotation;

    num_rotations = (shd->flags & 1) != 0 ? 1 : 8;
    for (rotation = 0; rotation < num_rotations; rotation++) {
        if (shd->field_48[rotation] != NULL) {
            FREE(shd->field_48[rotation]);
        }
        if (shd->field_8[rotation] != NULL) {
            FREE(shd->field_8[rotation]);
        }
    }
}

// 0x51BFB0
int sub_51BFB0(FILE* stream, uint8_t* pixels, int width, int height, int pitch, TigArtHeader* hdr, int rotation, int frame)
{
    int data_size;
    int index;

    data_size = sub_51C080(stream, pixels, width, height, pitch);
    if (data_size == -1) {
        return TIG_ERR_16;
    }

    if (data_size >= width * height) {
        for (index = 0; index < height; ++index) {
            if (fwrite(pixels, 1, width, stream) != (size_t)width) {
                return TIG_ERR_16;
            }

            pixels += pitch;
        }

        data_size = width * height;
    }

    hdr->data_size[rotation] += data_size;
    hdr->frames_tbl[rotation][frame].data_size += data_size;

    return TIG_OK;
}

// 0x51C080
int sub_51C080(FILE* stream, uint8_t* pixels, int width, int height, int pitch)
{
    long saved_offset;
    bool start_new_block = true;
    int size;
    int x;
    int y;
    uint8_t px;
    uint8_t run_length;
    uint8_t buffer_length;
    uint8_t buffer[128];

    saved_offset = ftell(stream);
    if (saved_offset == -1) {
        return -1;
    }

    size = 0;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (start_new_block) {
                // Start a new block, which can be either run-length encoded
                // extent (same color), or raw colors extent (different colors).
                // It will be determined on the next iterations.
                buffer[0] = *pixels;
                buffer_length = 1;
                run_length = 1;
                start_new_block = false;
            } else {
                if (*pixels == px) {
                    // Colors match - this color is a part of run-length
                    // encoded extent.

                    // Dump previous raw colors extent if it exists.
                    if (buffer_length > 1) {
                        if (fputc((buffer_length - 1) | 0x80, stream) == -1) {
                            return -1;
                        }

                        if (fwrite(buffer, 1, buffer_length - 1, stream) != (size_t)(buffer_length - 1)) {
                            return -1;
                        }

                        size += buffer_length;
                    }

                    buffer_length = 0;
                    run_length++;

                    // Check if run-length encoded extent reached max length.
                    if (run_length == 127) {
                        if (fputc(127, stream) == -1) {
                            return -1;
                        }

                        if (fputc(px, stream) == -1) {
                            return -1;
                        }

                        // Run-length
                        size += 2;
                        start_new_block = true;
                    }
                } else {
                    // Colors are different, this color is a part of raw colors
                    // extent.

                    // Dump previous run-length encoded extent.
                    if (run_length > 1) {
                        if (fputc(run_length, stream) == -1) {
                            return -1;
                        }

                        if (fputc(px, stream) == -1) {
                            return -1;
                        }

                        run_length = 1;
                        size += 2;
                    }

                    buffer[buffer_length++] = px;

                    // Check of raw colors extent reached max length.
                    if (buffer_length == 127) {
                        if (fputc(buffer_length | 0x80, stream) == -1) {
                            return -1;
                        }

                        if (fwrite(buffer, 1, buffer_length, stream) != (size_t)buffer_length) {
                            return -1;
                        }

                        size += buffer_length + 1;
                        start_new_block = true;
                    }
                }
            }
            px = *pixels++;
        }
        pixels += pitch - width;
    }

    if (!start_new_block) {
        if (buffer_length > 1) {
            // Dump final raw colors extent.
            if (fputc(buffer_length | 0x80, stream) == -1) {
                return -1;
            }

            if (fwrite(buffer, 1, buffer_length, stream) != (size_t)buffer_length) {
                return -1;
            }

            size += buffer_length + 1;
        } else {
            // Dump final run-length encoded extent.
            if (fputc(run_length, stream) == -1) {
                return -1;
            }

            if (fputc(px, stream) == -1) {
                return -1;
            }

            size += 2;
        }
    }

    if (size < height * width) {
        return size;
    }

    if (fseek(stream, saved_offset, SEEK_SET) != 0) {
        return -1;
    }

    return height * width;
}

// 0x51C3C0
int sub_51C3C0(FILE* stream, uint8_t* pixels, int width, int height, int pitch, int* a6)
{
    int x;
    int y;
    int index;
    int v1;
    int v2[4];
    uint8_t* v3;
    uint8_t* v4;

    v1 = width / 4;

    v2[0] = 6;
    v2[1] = 4;
    v2[2] = 2;
    v2[0] = 0;

    v3 = (uint8_t*)MALLOC(v1);

    for (y = 0; y < height; ++y) {
        index = 0;

        if (v1 > 0) {
            memset(v3, 0, v1);
        }

        v4 = v3;
        for (x = 0; x < width; x++) {
            if (pixels[x] >= 4) {
                FREE(v2);
                return TIG_ERR_16;
            }

            *v4 |= (pixels[x] << v2[index]);

            if (++index == 4) {
                index = 0;
                v4++;
            }
        }

        if (fwrite(v3, 1, v1, stream) != (size_t)v1) {
            FREE(v2);
            return TIG_ERR_16;
        }

        pixels += pitch;
    }

    *a6 += height * v1;

    FREE(v2);
    return TIG_OK;
}

// 0x51C4E0
int sub_51C4E0(int a1, TigBmp* bmp, TigRect* content_rect, int* pitch_ptr, int* a5, int* a6, int* a7, bool a8, bool a9)
{
    TigRect rect;
    bool v1;
    int v2;
    int v3;

    *a7 = 0;

    if (a8) {
        v1 = false;
    } else if (bmp->width > 0) {
        int x;
        int y;
        int v4 = 0;
        int v5;
        int v6;
        int v7;

        for (x = 0; x < bmp->width; ++x) {
            if (v4 >= 2) {
                break;
            }

            for (y = 0; y < bmp->height; ++y) {
                if (v4 >= 2) {
                    break;
                }

                if (((uint8_t*)bmp->pixels)[bmp->pitch * y + x] == 1) {
                    v4++;
                    v2 = x;
                    v3 = y;
                }
            }
        }

        v5 = 1;
        if (v4 == 1) {
            v1 = true;
            v6 = v2 != 0 ? v2 - 1 : 1;
            v7 = v3 != 0 ? v3 - 1 : 1;

            if (a9) {
                ((uint8_t*)bmp->pixels)[bmp->pitch * v3 + v2] = 0;
            } else {
                ((uint8_t*)bmp->pixels)[bmp->pitch * v3 + v2] = ((uint8_t*)bmp->pixels)[bmp->pitch * v7 + v6];
            }

            *a7 = 1;
        }
    }

    if (*a7 == 0) {
        if (a1 > 0) {
            v1 = true;
            v2 = *a5 + content_rect->x;
            v3 = *a6 + content_rect->y;
        } else {
            v1 = false;
        }
    }

    rect.x = 0;
    rect.y = 0;
    rect.width = bmp->width;
    rect.height = bmp->height;
    sub_51C6D0(bmp->pixels, &rect, bmp->pitch, content_rect);

    if (content_rect->width == 0 || content_rect->height == 0) {
        return TIG_ERR_16;
    }

    *pitch_ptr = bmp->pitch;

    if (v1) {
        *a5 = v2 - content_rect->x;
        *a6 = v3 - content_rect->y;
    } else {
        *a5 = content_rect->width / 2;
        *a6 = content_rect->height - 1;
    }

    return TIG_OK;
}

// 0x51C6D0
void sub_51C6D0(uint8_t* pixels, const TigRect* rect, int pitch, TigRect* content_rect)
{
    int tgt;
    uint8_t* stride;
    int x;
    int y;

    *content_rect = *rect;
    content_rect->width = 0;
    content_rect->height = 0;

    tgt = -1;
    for (y = rect->y; y < rect->y + rect->height; ++y) {
        if (tgt != -1) {
            break;
        }

        stride = pixels + pitch * y;
        for (x = rect->x; x < rect->x + rect->width; ++x) {
            if (stride[x] != 0) {
                tgt = y;
                break;
            }
        }
    }

    // NOTE: Probably a bug - if it does not have empty space on top, it skips
    // detecting empty space on other sides.
    if (tgt == -1) {
        return;
    }
    content_rect->y = tgt;

    tgt = -1;
    for (y = rect->y + rect->height - 1; y >= rect->y; --y) {
        if (tgt != -1) {
            break;
        }

        stride = pixels + pitch * y;
        for (x = rect->x; x < rect->x + rect->width; ++x) {
            if (stride[x] != 0) {
                tgt = y;
                break;
            }
        }
    }
    content_rect->height = tgt - content_rect->y + 1;

    tgt = -1;
    for (x = rect->x; x < rect->x + rect->width; ++x) {
        if (tgt != -1) {
            break;
        }

        stride = pixels + x;
        for (y = rect->y; y < rect->y + rect->height; ++y) {
            if (stride[0] != 0) {
                tgt = x;
                break;
            }
            stride += pitch;
        }
    }
    content_rect->x = tgt;

    tgt = -1;
    for (x = rect->x + rect->width - 1; x >= rect->x; --x) {
        if (tgt != -1) {
            break;
        }

        stride = pixels + x;
        for (y = rect->y; y < rect->y + rect->height; ++y) {
            if (stride[0] != 0) {
                tgt = x;
                break;
            }
            stride += pitch;
        }
    }
    content_rect->width = tgt - content_rect->x + 1;
}

// 0x51C890
int sub_51C890(int frame, TigBmp* bmp, TigRect* content_rect, int* pitch_ptr, int* width_ptr, int* height_ptr)
{
    TigRect r1;

    *pitch_ptr = bmp->pitch;

    if (frame == 0) {
        TigRect r2;

        r1.x = 0;
        r1.y = 0;
        r1.width = sub_51CA80(bmp->pixels, bmp->pitch, bmp->height, 0);
        r1.height = bmp->height;

        sub_51C6D0(bmp->pixels, &r1, *pitch_ptr, content_rect);
        if (content_rect->width == 0 || content_rect->height == 0) {
            return TIG_ERR_16;
        }

        *width_ptr = content_rect->width;
        r1.x = content_rect->x + 1;
        r1.y = content_rect->y;
        r1.width = content_rect->width - 1;
        r1.height = content_rect->height;
        sub_51C6D0(bmp->pixels, &r1, *pitch_ptr, &r2);

        *height_ptr = r2.y - r1.y;
    } else {
        int saved_y = content_rect->y;
        int saved_height = content_rect->height;

        r1.x = content_rect->width < *width_ptr ? content_rect->x + *width_ptr : content_rect->x + content_rect->width;
        r1.y = 0;
        r1.width = sub_51CA80(bmp->pixels, bmp->pitch, bmp->height, r1.x) - r1.x;
        r1.height = bmp->height;

        sub_51C6D0(bmp->pixels, &r1, *pitch_ptr, content_rect);
        if (content_rect->width == 0 || content_rect->height == 0) {
            return TIG_ERR_16;
        }

        *width_ptr = content_rect->width;

        if (((uint8_t*)bmp->pixels)[*pitch_ptr * content_rect->y + content_rect->x] == 1) {
            int x;

            for (x = content_rect->x; x < content_rect->x + content_rect->width; ++x) {
                if (((uint8_t*)bmp->pixels)[*pitch_ptr * content_rect->y + content_rect->x] != 1) {
                    break;
                }

                ((uint8_t*)bmp->pixels)[*pitch_ptr * content_rect->y + content_rect->x] = 0;
            }

            *width_ptr = x - content_rect->x;
            ++content_rect->y;
        }

        content_rect->y = saved_y;
        content_rect->height = saved_height;
    }

    return TIG_OK;
}

// Calculate width of empty space.
//
// 0x51CA80
int sub_51CA80(uint8_t* pixels, int pitch, int height, int start)
{
    int x;
    int y;
    uint8_t* stride;
    bool stop = false;

    for (x = start; x < pitch; ++x) {
        stride = pixels + x;
        for (y = 0; y < height; ++y) {
            if (*stride != 0) {
                stop = true;
                break;
            }

            stride += pitch;
        }

        if (y == height && stop) {
            break;
        }
    }

    return x;
}
