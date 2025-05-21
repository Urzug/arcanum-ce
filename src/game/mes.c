#include "game/mes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tig/tig.h>

typedef struct MesFile {
    /* 0000 */ char path[TIG_MAX_PATH];
    /* 0104 */ int refcount;
    /* 0108 */ int num_entries;
    /* 010C */ int max_entries;
    /* 0110 */ MesFileEntry* entries;
    /* 0114 */ char* data;
    /* 0118 */ size_t size;
} MesFile;

static bool find_mes_file(const char* path, int* index_ptr);
static void mes_load_internal(MesFile* mes_file);
static bool parse_entry(TigFile* stream, MesFileEntry* mes_file_entry);
static bool parse_field(TigFile* stream, char* buffer);
static TigFile* mes_file_open(const char* path, const char* mode);
static void mes_file_close(TigFile* stream);
static int consume_next_char(TigFile* stream);
static int compare_mes_file_entry(const void* a, const void* b);
static void copy_mes_file_entry(MesFileEntry* dst, MesFileEntry* src, char* str);
static void check_duplicates(MesFile* mes_file);

// 0x6018DC
static int mes_file_parse_line;

// 0x6018E0
static char byte_6018E0[MAX_STRING];

// 0x6020B0
static const char* current_mes_file_path;

// 0x6020B4
static int mes_file_parse_end;

// 0x6020B8
static char byte_6020B8[MAX_STRING];

// 0x602888
static int mes_file_parse_pos;

// 0x60288C
static int mes_files_length;

// 0x602890
static int mes_files_capacity;

// 0x602894
static MesFile* mes_files;

// 0x4D4290
bool mes_load(const char* path, mes_file_handle_t* mes_file_handle_ptr)
{
    int index;
    MesFile mes_file;

    if (find_mes_file(path, &index)) {
        // File has already been loaded, increment refcount.
        mes_files[index].refcount++;
        *mes_file_handle_ptr = (mes_file_handle_t)index;
        return true;
    }

    // Initialize new mes file object.
    strcpy(mes_file.path, path);
    mes_file.refcount = 1;
    mes_file.num_entries = 0;
    mes_file.max_entries = 0;
    mes_file.entries = NULL;

    // Keep current path for error reporting.
    current_mes_file_path = path;
    mes_load_internal(&mes_file);

    if (mes_file.num_entries == 0) {
        if (mes_file_handle_ptr != NULL) {
            *mes_file_handle_ptr = MES_FILE_HANDLE_INVALID;
        }
        return false;
    }

    mes_files[index] = mes_file;
    mes_files_length++;

    *mes_file_handle_ptr = (mes_file_handle_t)index;

    return true;
}

// 0x4D43A0
void mes_get_msg(mes_file_handle_t mes_file_handle, MesFileEntry* mes_file_entry)
{
    MesFileEntry* found;

    found = bsearch(mes_file_entry,
        mes_files[mes_file_handle].entries,
        mes_files[mes_file_handle].num_entries,
        sizeof(MesFileEntry),
        compare_mes_file_entry);

    if (found != NULL) {
        *mes_file_entry = *found;
    } else {
        sprintf(byte_6020B8, "Error! Missing line %d in %s",
            mes_file_entry->num,
            mes_files[mes_file_handle].path);
        mes_file_entry->str = byte_6020B8;
    }
}

// 0x4D4420
bool mes_find_first(mes_file_handle_t mes_file_handle, MesFileEntry* mes_file_entry)
{
    if (mes_files[mes_file_handle].num_entries == 0) {
        return false;
    }

    *mes_file_entry = mes_files[mes_file_handle].entries[0];

    return true;
}

// 0x4D4460
bool mes_find_next(mes_file_handle_t mes_file_handle, MesFileEntry* mes_file_entry)
{
    MesFileEntry* found;
    int exists;
    int index;

    found = tig_bsearch(mes_file_entry,
        mes_files[mes_file_handle].entries,
        mes_files[mes_file_handle].num_entries,
        sizeof(MesFileEntry),
        compare_mes_file_entry,
        &exists);
    index = (int)(found - mes_files[mes_file_handle].entries);
    if (exists) {
        index++;
    }
    if (index >= mes_files[mes_file_handle].num_entries) {
        return false;
    }

    *mes_file_entry = mes_files[mes_file_handle].entries[index];

    return true;
}

// 0x4D44E0
int mes_entries_count(mes_file_handle_t mes_file_handle)
{
    return mes_files[mes_file_handle].num_entries;
}

// 0x4D4500
bool mes_get_entry(mes_file_handle_t mes_file_handle, int index, MesFileEntry* mes_file_entry)
{
    if (index < 0 || index >= mes_files[mes_file_handle].num_entries) {
        return false;
    }

    *mes_file_entry = mes_files[mes_file_handle].entries[index];

    return true;
}

// 0x4D4550
int mes_entries_count_in_range(mes_file_handle_t mes_file_handle, int start, int end)
{
    MesFileEntry mes_file_entry;
    MesFileEntry* found;
    int index;
    int cnt = 0;

    mes_file_entry.num = start;
    found = bsearch(&mes_file_entry,
        mes_files[mes_file_handle].entries,
        mes_files[mes_file_handle].num_entries,
        sizeof(MesFileEntry),
        compare_mes_file_entry);
    if (found != NULL) {
        index = (int)(found - mes_files[mes_file_handle].entries);
        while (index < mes_files[mes_file_handle].num_entries) {
            if (mes_files[mes_file_handle].entries[index].num > end) {
                break;
            }
            index++;
            cnt++;
        }
    }

    return cnt;
}

// 0x4D45E0
bool mes_search(mes_file_handle_t mes_file_handle, MesFileEntry* mes_file_entry)
{
    MesFileEntry* found;

    found = bsearch(mes_file_entry,
        mes_files[mes_file_handle].entries,
        mes_files[mes_file_handle].num_entries,
        sizeof(MesFileEntry),
        compare_mes_file_entry);
    if (found == NULL) {
        return false;
    }

    *mes_file_entry = *found;

    return true;
}

// 0x4D4630
bool mes_unload(mes_file_handle_t mes_file_handle)
{
    if (mes_file_handle != MES_FILE_HANDLE_INVALID) {
        mes_files[mes_file_handle].refcount--;
        if (mes_files[mes_file_handle].refcount == 0) {
            FREE(mes_files[mes_file_handle].entries);
            FREE(mes_files[mes_file_handle].data);
            mes_files_length--;
        }

        if (mes_files_length == 0) {
            FREE(mes_files);
            mes_files = NULL;
            mes_files_capacity = 0;
        }
    }
    return true;
}

// 0x4D46D0
int mes_num_entries(mes_file_handle_t mes_file_handle)
{
    return mes_files[mes_file_handle].num_entries;
}

// 0x4D46F0
void mes_merge(mes_file_handle_t dst, mes_file_handle_t src)
{
    int index;
    char* data;
    MesFile* dst_mes_file = &(mes_files[dst]);
    MesFile* src_mes_file = &(mes_files[src]);

    data = dst_mes_file->data;

    dst_mes_file->data = (char*)REALLOC(dst_mes_file->data,
        dst_mes_file->size + src_mes_file->size);
    dst_mes_file->entries = (MesFileEntry*)REALLOC(dst_mes_file->entries,
        sizeof(MesFileEntry) * (dst_mes_file->num_entries + src_mes_file->num_entries));

    memcpy(&(dst_mes_file->data[dst_mes_file->size]),
        src_mes_file->data,
        src_mes_file->size);
    memcpy(&(dst_mes_file->entries[dst_mes_file->num_entries]),
        src_mes_file->entries,
        sizeof(MesFileEntry) * src_mes_file->num_entries);

    for (index = 0; index < dst_mes_file->num_entries; index++) {
        dst_mes_file->entries[index].str += dst_mes_file->data - data;
    }

    for (index = 0; index < src_mes_file->num_entries; index++) {
        dst_mes_file->entries[dst_mes_file->num_entries + index].str += dst_mes_file->data
            + dst_mes_file->size
            - src_mes_file->data;
    }

    dst_mes_file->size += src_mes_file->size;
    dst_mes_file->num_entries += src_mes_file->num_entries;

    if (dst_mes_file->num_entries != 0) {
        qsort(dst_mes_file->entries,
            dst_mes_file->num_entries,
            sizeof(MesFileEntry),
            compare_mes_file_entry);
    }

    check_duplicates(dst_mes_file);
}

// 0x4D4840
bool find_mes_file(const char* path, int* index_ptr)
{
    int index;
    int candidate = -1;

    for (index = 0; index < mes_files_capacity; index++) {
        if (mes_files[index].refcount != 0) {
            if (strcmpi(path, mes_files[index].path) == 0) {
                *index_ptr = index;
                return true;
            }
        } else {
            if (candidate == -1) {
                candidate = index;
            }
        }
    }

    if (candidate != -1) {
        *index_ptr = candidate;
        return false;
    }

    *index_ptr = index;

    mes_files_capacity += 10;
    mes_files = (MesFile*)REALLOC(mes_files, sizeof(MesFile) * mes_files_capacity);

    while (index < mes_files_capacity) {
        mes_files[index++].refcount = 0;
    }

    return false;
}

// 0x4D4920
void mes_load_internal(MesFile* mes_file)
{
    TigFile* stream;
    MesFileEntry mes_file_entry;
    char buffer[MAX_STRING];
    int offset;

    stream = mes_file_open(mes_file->path, "rt");
    if (stream == NULL) {
        return;
    }

    mes_file->size = tig_file_filelength(stream);
    mes_file->data = (char*)MALLOC(mes_file->size);

    mes_file_entry.str = buffer;

    offset = 0;
    mes_file_parse_line = 1;

    while (parse_entry(stream, &mes_file_entry)) {
        if (mes_file->num_entries == mes_file->max_entries) {
            mes_file->max_entries += 10;
            mes_file->entries = (MesFileEntry*)REALLOC(mes_file->entries, sizeof(MesFileEntry) * mes_file->max_entries);
        }

        copy_mes_file_entry(&(mes_file->entries[mes_file->num_entries]),
            &mes_file_entry,
            &(mes_file->data[offset]));
        offset += (int)strlen(mes_file_entry.str) + 1;
        mes_file->num_entries++;
    }

    mes_file->size = offset;
    mes_file_close(stream);

    if (mes_file->num_entries != 0) {
        qsort(mes_file->entries, mes_file->num_entries, sizeof(MesFileEntry), compare_mes_file_entry);
    }

    check_duplicates(mes_file);
}

// 0x4D4A60
bool parse_entry(TigFile* stream, MesFileEntry* mes_file_entry)
{
    char buffer[MAX_STRING];

    if (!parse_field(stream, buffer)) {
        return false;
    }

    if (!SDL_isdigit(buffer[0])) {
        tig_debug_printf("Warning! Bad number field on line: %d (%s)\n",
            mes_file_parse_line,
            current_mes_file_path);
    }

    mes_file_entry->num = atoi(buffer);

    if (!parse_field(stream, buffer)) {
        tig_debug_printf("Warning! Missing text on line: %d (%s)\n",
            mes_file_parse_line,
            current_mes_file_path);
        return false;
    }

    strcpy(mes_file_entry->str, buffer);

    return true;
}

// 0x4D4B50
bool parse_field(TigFile* stream, char* buffer)
{
    int ch;
    int pos;
    bool too_long;

    ch = consume_next_char(stream);
    while (ch != '{') {
        if (ch == -1) {
            return false;
        }

        if (ch == '\n') {
            mes_file_parse_line++;
        } else if (ch == '}') {
            tig_debug_printf("Warning! Possible missing left brace { on or before line %d (%s)\n",
                mes_file_parse_line,
                current_mes_file_path);
        }

        ch = consume_next_char(stream);
    }

    pos = 0;
    too_long = false;

    ch = consume_next_char(stream);
    while (ch != '}') {
        if (ch == -1) {
            tig_debug_printf("Warning! Expected right bracket }, reached end of file (%s).\n",
                current_mes_file_path);
            return false;
        }

        if (ch == '\n') {
            mes_file_parse_line++;
        } else if (ch == '{') {
            tig_debug_printf("Warning! Possible missing right brace } on or before line %d (%s)\n",
                mes_file_parse_line,
                current_mes_file_path);
        }

        if (pos < MAX_STRING) {
            buffer[pos++] = (char)ch;
        } else {
            too_long = true;
        }

        ch = consume_next_char(stream);
    }

    buffer[pos] = '\0';

    if (too_long) {
        tig_debug_printf("Warning! String too long on line: %d (%s)\n",
            mes_file_parse_line,
            current_mes_file_path);
    }

    return true;
}

// 0x4D4C70
TigFile* mes_file_open(const char* path, const char* mode)
{
    mes_file_parse_pos = 0;
    mes_file_parse_end = 0;
    return tig_file_fopen(path, mode);
}

// 0x4D4C90
void mes_file_close(TigFile* stream)
{
    tig_file_fclose(stream);
}

// 0x4D4CA0
int consume_next_char(TigFile* stream)
{
    if (mes_file_parse_pos == mes_file_parse_end) {
        mes_file_parse_pos = 0;
        mes_file_parse_end = 0;

        if (tig_file_fgets(byte_6018E0, sizeof(byte_6018E0), stream) == NULL) {
            return -1;
        }

        mes_file_parse_end = (int)strlen(byte_6018E0);
        if (mes_file_parse_end == 0) {
            return -1;
        }
    }

    return byte_6018E0[mes_file_parse_pos++];
}

// 0x4D4D10
int compare_mes_file_entry(const void* a, const void* b)
{
    if (((MesFileEntry*)a)->num < ((MesFileEntry*)b)->num) {
        return -1;
    } else if (((MesFileEntry*)a)->num > ((MesFileEntry*)b)->num) {
        return 1;
    } else {
        return 0;
    }
}

// 0x4D4D30
void copy_mes_file_entry(MesFileEntry* dst, MesFileEntry* src, char* str)
{
    *dst = *src;
    dst->str = str;
    strcpy(str, src->str);
}

// 0x4D4D70
void check_duplicates(MesFile* mes_file)
{
    int index1;
    int index2;

    for (index1 = 0; index1 < mes_file->num_entries; index1++) {
        for (index2 = 0; index2 < index1; index2++) {
            if (mes_file->entries[index1].num == mes_file->entries[index2].num) {
                tig_debug_printf("%s: two lines numbered %d\n",
                    mes_file->path,
                    index1);
            }
        }
    }
}

// 0x4D4DD0
void mes_stats()
{
    int index;

    for (index = 0; index < mes_files_capacity; index++) {
        if (mes_files[index].refcount > 0) {
            tig_debug_printf("Open Msg File: \"%s\", ref_count=%d.\n",
                mes_files[index].path,
                mes_files[index].refcount);
        }
    }
}
