#include "tig/file.h"

#include <direct.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fpattern.h>

#include "tig/database.h"
#include "tig/debug.h"
#include "tig/find_file.h"
#include "tig/memory.h"
#include "tig/tig.h"

#define CACHE_DIR_NAME "TIGCache"
#define COPY_BUFFER_SIZE 0x8000

#define TIG_FILE_DATABASE 0x01
#define TIG_FILE_PLAIN 0x02
#define TIG_FILE_DELETE_ON_CLOSE 0x04

typedef struct TigFile {
    /* 0000 */ char* path;
    /* 0004 */ unsigned int flags;
    /* 0008 */ union {
        TigDatabaseFileHandle* database_file_stream;
        FILE* plain_file_stream;
    } impl;
} TigFile;

// See 0x530BD0.
static_assert(sizeof(TigFile) == 0xC, "wrong size");

#define TIG_FILE_REPOSITORY_DATABASE 0x1
#define TIG_FILE_REPOSITORY_DIRECTORY 0x2

typedef struct TigFileRepository {
    int type;
    char* path;
    TigDatabase* database;
    struct TigFileRepository* next;
} TigFileRepository;

// See 0x52ED40.
static_assert(sizeof(TigFileRepository) == 0x10, "wrong size");

#define TIG_FILE_IGNORE_DATABASE 0x1
#define TIG_FILE_IGNORE_DIRECTORY 0x2

typedef struct TigFileIgnore {
    /* 0000 */ char* path;
    /* 0004 */ unsigned int flags;
    /* 0008 */ struct TigFileIgnore* next;
} TigFileIgnore;

// See 52F370.
static_assert(sizeof(TigFileIgnore) == 0xC, "wrong size");

static bool sub_52E840(const char* dst, const char* src);
static bool sub_52E8D0(TigFile* dst_stream, TigFile* src_stream);
static bool sub_52E900(TigFile* dst_stream, TigFile* src_stream, size_t size);
static bool sub_52E9C0(const char* path, TigFile* stream1, TigFile* stream2);
static TigFile* tig_file_create();
static void tig_file_destroy(TigFile* stream);
static bool tig_file_close_internal(TigFile* stream);
static int tig_file_open_internal(const char* path, const char* mode, TigFile* stream);
static void tig_file_process_attribs(unsigned int attribs, unsigned int* flags);
static void tig_file_list_add(TigFileList* list, TigFileInfo* info);
static unsigned int tig_file_ignored(const char* path);
static bool tig_file_copy_internal(TigFile* dst, TigFile* src);
static int tig_file_rmdir_recursively(const char* path);

// 0x62B2A8
static TigFileIgnore* off_62B2A8;

// 0x62B2AC
static TigFileRepository* tig_file_repositories_head;

// 0x62B2B0
static TigFileIgnore* tig_file_ignore_head;

// 0x52DFE0
bool tig_file_mkdir(const char* path)
{
    return tig_file_mkdir_ex(path) == 0;
}

// 0x52E000
bool tig_file_rmdir(const char* path)
{
    if (!tig_file_is_directory(path)) {
        return false;
    }

    if (!tig_file_is_empty_directory(path)) {
        return false;
    }

    if (!tig_file_rmdir_ex(path)) {
        return false;
    }

    return true;
}

// 0x52E040
bool sub_52E040(const char* path)
{
    bool success;
    char pattern[_MAX_PATH];
    TigFileList list;
    unsigned int index;

    if (!tig_file_is_directory(path)) {
        return false;
    }

    success = true;
    sprintf(pattern, "%s\\*.*", path);
    tig_file_list_create(&list, pattern);

    for (index = 0; index < list.count; index++) {
        sprintf(pattern, "%s\\%s", path, list.entries[index].path);
        if ((list.entries[index].attributes & TIG_FILE_ATTRIBUTE_SUBDIR) != 0) {
            if (strcmp(list.entries[index].path, ".") == 0
                || strcmp(list.entries[index].path, "..") == 0) {
                if (!sub_52E040(pattern)) {
                    success = false;
                }

                if (tig_file_rmdir_ex(pattern) != 0) {
                    success = false;
                }
            }
        } else {
            if (tig_file_remove(pattern) != 0) {
                success = false;
            }
        }
    }

    tig_file_list_destroy(&list);
    return success;
}

// 0x52E1B0
bool tig_file_is_empty_directory(const char* path)
{
    char pattern[_MAX_PATH];
    TigFileList list;
    bool is_empty;

    if (!tig_file_is_directory(path)) {
        return false;
    }

    sprintf(pattern, "%s\\*.*", path);

    tig_file_list_create(&list, pattern);

    // Only contains trivia (`.` and `..`).
    is_empty = list.count <= 2;

    tig_file_list_destroy(&list);

    return is_empty;
}

// 0x52E220
bool tig_file_is_directory(const char* path)
{
    TigFileInfo info;
    return tig_file_exists(path, &info) && (info.attributes & TIG_FILE_ATTRIBUTE_SUBDIR) != 0;
}

// 0x52E260
bool sub_52E260(const char* dst, const char* src)
{
    char path1[_MAX_PATH];
    char path2[_MAX_PATH];
    TigFileList list;
    unsigned int index;

    sprintf(path1, "%s\\*.*", src);
    tig_file_list_create(&list, path1);

    if (list.count != 0) {
        if (!tig_file_mkdir(dst)) {
            // FIXME: Leaks `list`.
            return false;
        }

        for (index = 0; index < list.count; index++) {
            sprintf(path1, "%s\\%s", src, list.entries[index].path);
            sprintf(path2, "%s\\%s", dst, list.entries[index].path);

            if ((list.entries[index].attributes & TIG_FILE_ATTRIBUTE_SUBDIR) != 0) {
                if (strcmp(list.entries[index].path, ".") == 0
                    || strcmp(list.entries[index].path, "..") == 0) {
                    if (!sub_52E260(path2, path1)) {
                        tig_file_list_destroy(&list);
                        return false;
                    }
                }
            } else {
                if (!sub_52E840(path2, path1)) {
                    tig_file_list_destroy(&list);
                    return false;
                }
            }
        }
    }

    tig_file_list_destroy(&list);
    return true;
}

// 0x52E430
bool sub_52E430(const char* dst, const char* src)
{
    char path1[_MAX_PATH];
    char path2[_MAX_PATH];
    TigFile* stream1;
    TigFile* stream2;
    bool success;
    int v1;

    if (!tig_file_is_directory(src)) {
        return false;
    }

    sprintf(path1, "%s.tfai", dst);
    stream1 = tig_file_fopen(path1, "wb");
    if (stream1 == NULL) {
        return false;
    }

    sprintf(path2, "%s.tfaf", dst);
    stream2 = tig_file_fopen(path2, "wb");
    if (stream2 == NULL) {
        tig_file_fclose(stream1);
        return false;
    }

    success = sub_52E9C0(src, stream1, stream2);
    if (success) {
        v1 = 3;
        if (tig_file_fwrite(&v1, sizeof(v1), 1, stream1) != 1) {
            success = false;
        }
    }

    tig_file_fclose(stream2);
    tig_file_fclose(stream1);

    if (!success) {
        tig_file_remove(path2);
        tig_file_remove(path1);
    }

    return success;
}

// 0x52E550
bool sub_52E550(const char* src, const char* dst)
{
    char path1[_MAX_PATH];
    char path2[_MAX_PATH];
    char path3[_MAX_PATH];
    TigFile* stream1;
    TigFile* stream2;
    int type;
    int size;
    char* pch;
    TigFile* tmp_stream;

    tig_file_mkdir(dst);

    sprintf(path1, "%s.tfai", src);
    stream1 = tig_file_fopen(path1, "rb");
    if (stream1 == NULL) {
        return false;
    }

    sprintf(path1, "%s.tfaf", src);
    stream2 = tig_file_fopen(path1, "rb");
    if (stream2 == NULL) {
        // FIXME: Leaks `stream1`.
        return false;
    }

    strcpy(path2, src);
    strcat(path2, "\\");

    while (tig_file_fread(&type, sizeof(type), 1, stream1) == 1) {
        if (type == 0) {
            if (tig_file_fread(&size, sizeof(size), 1, stream1) != 1) {
                break;
            }

            if (tig_file_fread(path1, size, 1, stream1) != 1) {
                break;
            }

            path1[size] = '\0';

            if (tig_file_fread(&size, sizeof(size), 1, stream1) != 1) {
                break;
            }

            sprintf(path3, "%s%s", path2, path1);
            tmp_stream = tig_file_fopen(path3, "wb");
            if (tmp_stream == NULL) {
                break;
            }

            if (!sub_52E900(tmp_stream, stream1, size)) {
                tig_file_fclose(tmp_stream);
                break;
            }

            tig_file_fclose(tmp_stream);
        } else if (type == 1) {
            if (tig_file_fread(&size, sizeof(size), 1, stream1) != 1) {
                break;
            }

            if (tig_file_fread(path1, size, 1, stream1) != 1) {
                break;
            }

            path1[size] = '\0';
            strcat(path2, path1);
            tig_file_mkdir(path2);
            strcat(path2, "\\");
        } else if (type == 2) {
            pch = strrchr(path2, '\\');
            if (pch == NULL) {
                break;
            }
            *pch = '\0';

            pch = strrchr(path2, '\\');
            if (pch == NULL) {
                break;
            }
            *pch = '\0';
        } else if (type == 3) {
            tig_file_fclose(stream2);
            tig_file_fclose(stream1);
            return true;
        }
    }

    tig_file_fclose(stream2);
    tig_file_fclose(stream1);
    return false;
}

// 0x52E840
bool sub_52E840(const char* dst, const char* src)
{
    TigFile* dst_stream;
    TigFile* src_stream;

    src_stream = tig_file_fopen(src, "rb");
    if (src_stream == NULL) {
        return false;
    }

    dst_stream = tig_file_fopen(dst, "wb");
    if (dst_stream == NULL) {
        tig_file_fclose(src_stream);
        return false;
    }

    if (!sub_52E8D0(dst_stream, src_stream)) {
        tig_file_fclose(dst_stream);
        tig_file_fclose(src_stream);
        tig_file_remove(dst);
        return false;
    }

    tig_file_fclose(dst_stream);
    tig_file_fclose(src_stream);
    return true;
}

// 0x52E8D0
bool sub_52E8D0(TigFile* dst_stream, TigFile* src_stream)
{
    size_t size = tig_file_filelength(src_stream);
    if (size != 0) {
        return sub_52E900(dst_stream, src_stream, size);
    } else {
        return true;
    }
}

// 0x52E900
bool sub_52E900(TigFile* dst_stream, TigFile* src_stream, size_t size)
{
    unsigned char buffer[COPY_BUFFER_SIZE];

    if (size == 0) {
        return false;
    }

    while (size > COPY_BUFFER_SIZE) {
        if (tig_file_fread(buffer, COPY_BUFFER_SIZE, 1, src_stream) != 1) {
            return false;
        }

        if (tig_file_fwrite(buffer, COPY_BUFFER_SIZE, 1, dst_stream) != 1) {
            return false;
        }

        size -= COPY_BUFFER_SIZE;
    }

    if (tig_file_fread(buffer, size, 1, src_stream) != 1) {
        return false;
    }

    if (tig_file_fwrite(buffer, size, 1, src_stream) != 1) {
        return false;
    }

    return true;
}

// 0x52E9C0
bool sub_52E9C0(const char* path, TigFile* stream1, TigFile* stream2)
{
    char pattern[_MAX_PATH];
    TigFileList list;
    unsigned int index;
    int v1;
    TigFile* tmp_stream;

    sprintf(pattern, "%s\\*.*", path);
    tig_file_list_create(&list, pattern);

    for (index = 0; index < list.count; index++) {
        if ((list.entries[index].attributes & TIG_FILE_ATTRIBUTE_SUBDIR) != 0) {
            if (strcmp(list.entries[index].path, ".") == 0
                || strcmp(list.entries[index].path, "..") == 0) {
                v1 = 1;
                if (tig_file_fwrite(&v1, sizeof(v1), 1, stream1) != 1) {
                    // FIXME: Leaks `list`.
                    return false;
                }

                v1 = (int)strlen(list.entries[index].path);
                if (tig_file_fwrite(&v1, sizeof(v1), 1, stream1) != 1) {
                    // FIXME: Leaks `list`.
                    return false;
                }

                if (tig_file_fputs(list.entries[index].path, stream1) < 0) {
                    // FIXME: Leaks `list`.
                    return false;
                }

                sprintf(pattern, "%s\\%s", path, list.entries[index].path);

                if (!sub_52E9C0(pattern, stream1, stream2)) {
                    // FIXME: Leaks `list`.
                    return false;
                }

                v1 = 2;
                if (tig_file_fwrite(&v1, sizeof(v1), 1, stream1) != 1) {
                    // FIXME: Leaks `list`.
                    return false;
                }
            }
        } else {
            v1 = 0;
            if (tig_file_fwrite(&v1, sizeof(v1), 1, stream1) != 1) {
                // FIXME: Leaks `list`.
                return false;
            }

            v1 = (int)strlen(list.entries[index].path);
            if (tig_file_fwrite(&v1, sizeof(v1), 1, stream1) != 1) {
                // FIXME: Leaks `list`.
                return false;
            }

            if (tig_file_fputs(list.entries[index].path, stream1) < 0) {
                // FIXME: Leaks `list`.
                return false;
            }

            sprintf(pattern, "%s\\%s", path, list.entries[index].path);

            tmp_stream = tig_file_fopen(pattern, "rb");
            if (tmp_stream == NULL) {
                // FIXME: Leaks `list`.
                return false;
            }

            if (!sub_52E8D0(stream2, tmp_stream)) {
                // FIXME: Leaks `tmp_stream`.
                // FIXME: Leaks `list`.
                return false;
            }

            tig_file_fclose(tmp_stream);
        }
    }

    tig_file_list_destroy(&list);
    return true;
}

// 0x52ECA0
int tig_file_init(TigContext* ctx)
{
    (void)ctx;

    TigFindFileData ffd;

    if (tig_find_first_file("tig.dat", &ffd)) {
        tig_file_repository_add("tig.dat");
    } else {
        tig_file_repository_add(tig_get_executable(false));
    }

    // Current working directory.
    tig_file_repository_add(".");

    // FIXME: Missing `tig_find_close`, leaking `ffd`.

    return TIG_OK;
}

// 0x52ED00
void tig_file_exit()
{
    TigFileIgnore* next;

    while (tig_file_ignore_head != NULL) {
        next = tig_file_ignore_head->next;
        if (tig_file_ignore_head->path != NULL) {
            FREE(tig_file_ignore_head->path);
        }
        FREE(tig_file_ignore_head);
        tig_file_ignore_head = next;
    }

    tig_file_repository_remove_all();
}

// 0x52ED40
bool tig_file_repository_add(const char* path)
{
    bool added = false;
    TigFileRepository* curr;
    TigFileRepository* prev;
    TigFileRepository* next;
    TigFileRepository* repo;
    TigFindFileData ffd;
    TigDatabase* database;
    char cache_path[_MAX_PATH];

    prev = NULL;
    curr = tig_file_repositories_head;
    while (curr != NULL && strcmpi(curr->path, path) != 0) {
        prev = curr;
        curr = curr->next;
    }

    if (curr != NULL) {
        if (prev != NULL) {
            if ((curr->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0
                && curr->next != NULL
                && strcmpi(curr->next->path, path) == 0) {
                // Move repo on top of the list.
                next = curr->next;
                curr->next = next->next;
                next->next = tig_file_repositories_head;
                tig_file_repositories_head = next;
            }

            prev->next = curr->next;
            curr->next = tig_file_repositories_head;
            tig_file_repositories_head = curr;
        }
        return true;
    }

    if (tig_find_first_file(path, &ffd)) {
        do {
            if ((ffd.find_data.attrib & _A_SUBDIR) != 0) {
                repo = (TigFileRepository*)MALLOC(sizeof(TigFileRepository));
                repo->type = 0;
                repo->path = NULL;
                repo->database = NULL;
                repo->next = NULL;

                repo->path = STRDUP(path);
                repo->type = TIG_FILE_REPOSITORY_DIRECTORY;
                repo->next = tig_file_repositories_head;
                tig_file_repositories_head = repo;

                tig_debug_printf("TIG File: Added path \"%s\"\n", path);
                added = true;
            } else {
                database = tig_database_open(path);
                if (database != NULL) {
                    repo = (TigFileRepository*)MALLOC(sizeof(TigFileRepository));
                    repo->type = 0;
                    repo->path = NULL;
                    repo->database = NULL;
                    repo->next = NULL;

                    repo->path = STRDUP(path);
                    repo->type = TIG_FILE_REPOSITORY_DATABASE;

                    if (added) {
                        repo->next = tig_file_repositories_head->next;
                        tig_file_repositories_head->next = repo;
                    } else {
                        repo->next = tig_file_repositories_head;
                        tig_file_repositories_head->next = repo;
                        added = true;
                    }

                    tig_debug_printf("TIG File: Added database \"%s\"\n", path);
                }
            }
        } while (tig_find_next_file(&ffd));

        // FIXME: Missing `tig_find_close`, leaking `ffd`.

        if (added) {
            if ((tig_file_repositories_head->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
                sprintf(cache_path, "%s\\%s", tig_file_repositories_head->path, CACHE_DIR_NAME);
                tig_file_rmdir_recursively(cache_path);
            }
            return true;
        }
    }

    tig_debug_printf("TIG File: Error - Invalid path \"%s\"\n", path);
    return false;
}

// 0x52EF30
bool tig_path_repository_remove(const char* file_name)
{
    TigFileRepository* repo;
    TigFileRepository* prev;
    TigFileRepository* next;
    bool removed = false;
    char path[_MAX_PATH];

    prev = NULL;
    repo = tig_file_repositories_head;
    while (repo != NULL) {
        if (strcmpi(file_name, repo->path) == 0) {
            next = repo->next;
            if ((repo->type & TIG_FILE_DATABASE) != 0) {
                tig_database_close(repo->database);
            } else {
                sprintf(path, "%s\\%s", repo->path, CACHE_DIR_NAME);
                tig_file_rmdir_recursively(path);
            }

            if (prev != NULL) {
                prev->next = next;
            } else {
                tig_file_repositories_head = next;
            }

            FREE(repo->path);
            FREE(repo);

            repo = next;
            removed = true;
        } else {
            prev = repo;
            repo = repo->next;
        }
    }

    return removed;
}

// 0x52F000
bool tig_file_repository_remove_all()
{
    TigFileRepository* curr;
    TigFileRepository* next;
    char path[_MAX_PATH];

    curr = tig_file_repositories_head;
    while (curr != NULL) {
        next = curr->next;
        if ((curr->type & 1) != 0) {
            tig_database_close(curr->database);
        } else {
            sprintf(path, "%s\\%s", curr->path, CACHE_DIR_NAME);
            tig_file_rmdir(path);
        }

        FREE(curr->path);
        FREE(curr);

        curr = next;
    }

    tig_file_repositories_head = NULL;

    return true;
}

// 0x52F0A0
bool tig_file_repository_guid(const char* path, GUID* guid)
{
    TigFileRepository* curr = tig_file_repositories_head;
    while (curr != NULL) {
        if ((curr->type & TIG_FILE_DATABASE) != 0) {
            if (strcmpi(curr->database->path, path) == 0) {
                *guid = curr->database->guid;
                return true;
            }
        }
        curr = curr->next;
    }

    return false;
}

// 0x52F100
int tig_file_mkdir_ex(const char* path)
{
    char temp_path[_MAX_PATH];
    size_t temp_path_length;
    TigFileRepository* repo;
    char* pch;

    temp_path[0] = '\0';

    if (path[0] != '.' && path[0] != '.' && path[1] != ':') {
        repo = tig_file_repositories_head;
        while (repo != NULL) {
            if ((repo->type & TIG_FILE_PLAIN) != 0) {
                strcpy(temp_path, repo->path);
                strcat(temp_path, "\\");
                break;
            }
            repo = repo->next;
        }
    }

    strcat(temp_path, path);

    temp_path_length = strlen(temp_path);
    if (temp_path[temp_path_length] == '\\') {
        temp_path[temp_path_length] = '\0';
    }

    pch = strchr(temp_path, '\\');
    while (pch != NULL) {
        *pch = '\0';
        _mkdir(temp_path);
        *pch = '\\';

        pch = strchr(pch + 1, '\\');
    }

    mkdir(temp_path);

    return 0;
}

// 0x52F230
int tig_file_rmdir_ex(const char* path)
{
    char temp_path[_MAX_PATH];
    TigFileRepository* repo;
    int rc = -1;

    temp_path[0] = '\0';

    if (path[0] != '.' && path[0] != '.' && path[1] != ':') {
        repo = tig_file_repositories_head;
        while (repo != NULL) {
            if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
                strcpy(temp_path, repo->path);
                strcat(temp_path, "\\");
                strcat(temp_path, path);

                if (rmdir(temp_path) == 0) {
                    rc = 0;
                }
            }
            repo = repo->next;
        }
    } else {
        strcat(temp_path, path);
        rc = rmdir(temp_path);
    }

    return rc;
}

// 0x52F370
void tig_file_ignore(const char* path, unsigned int flags)
{
    TigFileIgnore* ignore;
    size_t path_length;
    char* pch;

    if ((flags & (TIG_FILE_DATABASE | TIG_FILE_PLAIN)) != 0) {
        if (fpattern_isvalid(path)) {
            ignore = (TigFileIgnore*)MALLOC(sizeof(*ignore));
            ignore->flags = flags;
            ignore->path = strdup(path);
            ignore->next = tig_file_ignore_head;
            tig_file_ignore_head = ignore;

            // Trim *.* pattern from the end of string.
            path_length = strlen(ignore->path);
            if (path_length > 3) {
                pch = &(ignore->path[path_length - 3]);
                if (strcmp(pch, "*.*") == 0) {
                    *pch = '\0';
                }
            }
        }
    }
}

// 0x52F410
bool tig_file_extract(const char* filename, char* path)
{
    TigDatabaseEntry* database_entry;
    TigFile* in;
    TigFile* out;
    TigFileRepository* first_directory_repo;
    TigFileRepository* repo;
    TigFindFileData ffd;
    char tmp[_MAX_PATH];
    unsigned int ignored;

    first_directory_repo = NULL;
    if (filename[0] == '.' || filename[0] == '\\' || filename[1] == ':') {
        strcpy(path, filename);
        return true;
    }

    strcpy(path, CACHE_DIR_NAME);
    strcat(path, '\\');
    strcat(path, filename);

    ignored = tig_file_ignored(filename);

    repo = tig_file_repositories_head;
    while (repo != NULL) {
        if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
            if (first_directory_repo == NULL) {
                first_directory_repo = repo;
            }

            if ((ignored & TIG_FILE_IGNORE_DIRECTORY) == 0) {
                // Check if file exists in a directory bundle.
                sprintf(tmp, "%s\\%s", repo->path, filename);
                if (tig_find_first_file(tmp, &ffd)) {
                    strcpy(path, tmp);
                    tig_find_close(&ffd);
                    return true;
                }
                tig_find_close(&ffd);

                // Check if file is already expanded.
                sprintf(tmp, "%s\\%s", repo->path, path);
                if (tig_find_first_file(tmp, &ffd)) {
                    strcpy(path, tmp);
                    tig_find_close(&ffd);
                    return true;
                }
                tig_find_close(&ffd);
            }
        } else if ((repo->type & TIG_FILE_REPOSITORY_DATABASE) != 0) {
            if ((ignored & TIG_FILE_IGNORE_DATABASE) == 0) {
                if (tig_database_get_entry(repo->database, filename, &database_entry)) {
                    _splitpath(path, NULL, tmp, NULL, NULL);
                    tig_file_mkdir_ex(tmp);

                    out = tig_file_fopen(path, "wb");
                    if (out == NULL) {
                        return false;
                    }

                    in = tig_file_create();
                    in->impl.database_file_stream = tig_database_fopen_entry(repo->database, database_entry, "rb");
                    in->flags |= TIG_FILE_DATABASE;

                    if (!tig_file_copy_internal(out, in)) {
                        tig_file_fclose(out);
                        tig_file_fclose(in);
                        return false;
                    }

                    tig_file_fclose(out);
                    tig_file_fclose(in);

                    if (first_directory_repo == NULL) {
                        // If we're here it means out file was sucessfully
                        // created in the first directory repo, we just need it
                        // to look deeper.
                        while (repo != NULL) {
                            if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
                                first_directory_repo = repo;
                                break;
                            }
                            repo = repo->next;
                        }
                    }

                    sprintf(tmp, "%s\\%s", first_directory_repo->path, path);
                    strcpy(path, tmp);

                    return true;
                }
            }
        }
        repo = repo->next;
    }

    return false;
}

// 0x52F760
void tig_file_list_create(TigFileList* list, const char* pattern)
{
    char mutable_pattern[_MAX_PATH];
    size_t pattern_length;
    TigFileInfo info;
    TigFindFileData directory_ffd;
    TigFileRepository* repo;
    unsigned int ignored;
    TigDatabaseFindFileData database_ffd;
    char path[_MAX_PATH];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    strcpy(mutable_pattern, pattern);

    pattern_length = strlen(mutable_pattern);
    if (pattern_length > 2 && strcmp(&(mutable_pattern[pattern_length - 3]), "*.*") == 0) {
        mutable_pattern[-3] = '\0';
    }

    list->count = 0;
    list->entries = NULL;

    if (mutable_pattern[0] == '.' || mutable_pattern[0] == '\\' || mutable_pattern[1] == ':') {
        if (tig_find_first_file(mutable_pattern, &directory_ffd)) {
            do {
                tig_file_process_attribs(directory_ffd.find_data.attrib, &(info.attributes));
                info.size = directory_ffd.find_data.size;
                strcpy(info.path, directory_ffd.find_data.name);
                info.modify_time = directory_ffd.find_data.time_write;

                tig_file_list_add(list, &info);
            } while (tig_find_next_file(&directory_ffd));
        }
        tig_find_close(&directory_ffd);
    } else {
        ignored = tig_file_ignored(pattern);

        repo = tig_file_repositories_head;
        while (repo != NULL) {
            if ((repo->type & TIG_FILE_REPOSITORY_DATABASE) != 0) {
                if ((ignored & TIG_FILE_IGNORE_DATABASE) == 0) {
                    if (tig_database_find_first_entry(repo->database, mutable_pattern, &database_ffd)) {
                        do {
                            info.attributes = TIG_FILE_ATTRIBUTE_0x80 | TIG_FILE_ATTRIBUTE_READONLY;
                            if ((database_ffd.is_directory & 0x1) != 0) {
                                info.attributes |= TIG_FILE_ATTRIBUTE_SUBDIR;
                            }
                            info.size = database_ffd.size;

                            _splitpath(database_ffd.name, NULL, NULL, fname, ext);
                            _makepath(info.path, NULL, NULL, fname, ext);

                            tig_file_list_add(list, &info);
                        } while (tig_database_find_next_entry(&database_ffd));
                        tig_database_find_close(&database_ffd);
                    }
                }
            } else if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
                if ((ignored & TIG_FILE_IGNORE_DIRECTORY) == 0) {
                    strcpy(path, repo->path);
                    strcat(path, "\\");
                    strcat(path, mutable_pattern);

                    if (tig_find_first_file(path, &directory_ffd)) {
                        do {
                            tig_file_process_attribs(directory_ffd.find_data.attrib, &(info.attributes));
                            info.size = directory_ffd.find_data.size;
                            strcpy(info.path, directory_ffd.find_data.name);
                            info.modify_time = directory_ffd.find_data.time_write;

                            tig_file_list_add(list, &info);
                        } while (tig_find_next_file(&directory_ffd));
                    }
                    tig_find_close(&directory_ffd);
                }
            }
            repo = repo->next;
        }
    }
}

// 0x52FB20
void tig_file_list_destroy(TigFileList* file_list)
{
    if (file_list->entries != NULL) {
        FREE(file_list->entries);
    }

    file_list->count = 0;
    file_list->entries = NULL;
}

// 0x52FB40
int tig_file_filelength(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_filelength(stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return _filelength(_fileno(stream->impl.plain_file_stream));
    }

    return -1;
}

// 0x52FB80
bool tig_file_exists(const char* file_name, TigFileInfo* info)
{
    TigFindFileData ffd;
    TigFileRepository* repo;
    TigDatabaseEntry* database_entry;
    unsigned int ignored;
    char path[_MAX_PATH];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    ignored = tig_file_ignored(file_name);

    if (file_name[0] == '.' || file_name[0] == '\\' || file_name[1] == ':') {
        if (!tig_find_first_file(file_name, &ffd)) {
            tig_find_close(&ffd);
            return false;
        }

        if (info != NULL) {
            tig_file_process_attribs(ffd.find_data.attrib, &(info->attributes));
            info->size = ffd.find_data.size;
            strcpy(info->path, ffd.find_data.name);
            info->modify_time = ffd.find_data.time_write;
        }

        tig_find_close(&ffd);
        return true;
    }

    repo = tig_file_repositories_head;
    while (repo != NULL) {
        if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
            if ((ignored & TIG_FILE_IGNORE_DIRECTORY) == 0) {
                strcpy(path, repo->path);
                strcat(path, "\\");
                strcat(path, file_name);

                if (tig_find_first_file(path, &ffd)) {
                    if (info != NULL) {
                        tig_file_process_attribs(ffd.find_data.attrib, &(info->attributes));
                        info->size = ffd.find_data.size;
                        strcpy(info->path, ffd.find_data.name);
                        info->modify_time = ffd.find_data.time_write;
                    }

                    tig_find_close(&ffd);
                    return true;
                }
                tig_find_close(&ffd);
            }
        } else if ((repo->type & TIG_FILE_REPOSITORY_DATABASE) != 0) {
            if ((ignored & TIG_FILE_IGNORE_DATABASE) == 0) {
                if (tig_database_get_entry(repo->database, file_name, &database_entry)) {
                    if (info != NULL) {
                        info->attributes = TIG_FILE_ATTRIBUTE_0x80 | TIG_FILE_ATTRIBUTE_READONLY;
                        if ((database_entry->flags & TIG_DATABASE_ENTRY_DIRECTORY) != 0) {
                            info->attributes |= TIG_FILE_ATTRIBUTE_SUBDIR;
                        }
                        info->size = database_entry->size;

                        _splitpath(database_entry->path, NULL, NULL, fname, ext);
                        _makepath(info->path, 0, 0, fname, ext);
                    }

                    return true;
                }
            }
        }
        repo = repo->next;
    }

    return false;
}

// 0x52FE60
bool tig_file_exists_in_path(const char* search_path, const char* file_name, TigFileInfo* info)
{
    TigFindFileData ffd;
    TigFileRepository* repo;
    TigDatabaseEntry* database_entry;
    unsigned int ignored;
    char path[_MAX_PATH];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    if (file_name[0] == '.' || file_name[0] == '\\' || file_name[1] == ':') {
        return tig_file_exists(file_name, info);
    }

    ignored = tig_file_ignored(file_name);

    repo = tig_file_repositories_head;
    while (repo != NULL) {
        if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
            if ((ignored & TIG_FILE_IGNORE_DATABASE) != 0
                && strcmpi(search_path, repo->path) == 0) {
                strcpy(path, repo->path);
                strcat(path, "\\");
                strcat(path, file_name);

                if (tig_find_first_file(path, &ffd)) {
                    if (info != NULL) {
                        tig_file_process_attribs(ffd.find_data.attrib, &(info->attributes));
                        info->size = ffd.find_data.size;
                        strcpy(info->path, ffd.find_data.name);
                        info->modify_time = ffd.find_data.time_write;
                    }

                    tig_find_close(&ffd);
                    return true;
                }
                tig_find_close(&ffd);
            }
        } else if ((repo->type & TIG_FILE_REPOSITORY_DATABASE) != 0) {
            if ((ignored & TIG_FILE_IGNORE_DATABASE) == 0
                && strcmpi(search_path, repo->path)) {
                if (tig_database_get_entry(repo->database, file_name, &database_entry)) {
                    if (info != NULL) {
                        info->attributes = TIG_FILE_ATTRIBUTE_0x80 | TIG_FILE_ATTRIBUTE_READONLY;
                        if ((database_entry->flags & TIG_DATABASE_ENTRY_DIRECTORY) != 0) {
                            info->attributes |= TIG_FILE_ATTRIBUTE_SUBDIR;
                        }
                        info->size = database_entry->size;

                        _splitpath(database_entry->path, NULL, NULL, fname, ext);
                        _makepath(info->path, 0, 0, fname, ext);
                    }

                    return true;
                }
            }
        }
        repo = repo->next;
    }

    return false;
}

// 0x5300F0
int tig_file_remove(const char* file_name)
{
    TigFileRepository* repo;
    char path[_MAX_PATH];
    TigDatabaseEntry* database_entry;

    if (file_name[0] == '.' || file_name[0] == '\\' || file_name[1] == ':') {
        return remove(file_name);
    }

    if ((tig_file_ignored(file_name) & 0x2) != 0) {
        return 1;
    }

    repo = tig_file_repositories_head;
    while (repo != NULL) {
        if ((repo->type & TIG_FILE_PLAIN) != 0) {
            sprintf(path, "%s\\%s", repo->path, file_name);

            if (remove(path) == 0) {
                repo = repo->next;
                while (repo != NULL) {
                    if ((repo->type & TIG_FILE_DATABASE) != 0
                        && tig_database_get_entry(repo->database, file_name, &database_entry)) {
                        database_entry->flags &= ~0x200;
                        database_entry->flags |= 0x100;
                        break;
                    }
                    repo = repo->next;
                }

                return 0;
            }
        }
        repo = repo->next;
    }

    return 1;
}

// 0x5301F0
int tig_file_rename(const char* old_file_name, const char* new_file_name)
{
    TigFileRepository* repo;
    char old_path[_MAX_PATH];
    char new_path[_MAX_PATH];

    if (old_file_name[0] == '.' || old_file_name[0] == '\\' || old_file_name[1] == ':') {
        return rename(old_file_name, new_file_name);
    }

    if ((tig_file_ignored(old_file_name) & 0x2) != 0) {
        return 1;
    }

    repo = tig_file_repositories_head;
    while (repo != NULL) {
        if ((repo->type & TIG_FILE_PLAIN) != 0) {
            sprintf(old_path, "%s\\%s", repo->path, old_file_name);
            sprintf(new_path, "%s\\%s", repo->path, new_file_name);

            if (rename(old_path, new_path) != 0) {
                return 0;
            }
        }
        repo = repo->next;
    }

    return 1;
}

// 0x5302D0
TigFile* tig_file_tmpfile()
{
    TigFile* stream;

    stream = tig_file_create();
    stream->flags |= TIG_FILE_DELETE_ON_CLOSE;
    stream->path = STRDUP(tig_file_tmpnam(NULL));

    if (tig_file_open_internal(stream->path, "wb", stream) == 0) {
        FREE(stream->path);
        tig_file_destroy(stream);
        return NULL;
    }

    return stream;
}

// 0x530320
char* tig_file_tmpnam(char* buffer)
{
    return tmpnam(buffer);
}

// 0x530330
int tig_file_fclose(TigFile* stream)
{
    int rc;

    rc = tig_file_close_internal(stream);
    tig_file_destroy(stream);

    if (rc != 0) {
        return false;
    }

    return true;
}

// 0x530360
int tig_file_fflush(TigFile* stream)
{
    if (stream == NULL) {
        return fflush(stdin);
    }

    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fflush(stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fflush(stream->impl.plain_file_stream);
    }

    return -1;
}

// 0x5303A0
TigFile* tig_file_fopen(const char* path, const char* mode)
{
    TigFile* stream;

    stream = tig_file_create();
    if (tig_file_open_internal(path, mode, stream) == 0) {
        tig_file_destroy(stream);
        return NULL;
    }

    return stream;
}

// 0x5303D0
TigFile* tig_file_reopen(const char* path, const char* mode, TigFile* stream)
{
    tig_file_close_internal(stream);

    if (tig_file_open_internal(path, mode, stream) == 0) {
        tig_file_destroy(stream);
        return NULL;
    }

    return stream;
}

// 0x530410
int tig_file_setbuf(TigFile* stream, char* buffer)
{
    if (buffer != NULL) {
        return tig_file_setvbuf(stream, buffer, _IOFBF, 512);
    } else {
        return tig_file_setvbuf(stream, NULL, _IONBF, 512);
    }
}

// 0x530440
int tig_file_setvbuf(TigFile* stream, char* buffer, int mode, size_t size)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_setvbuf(stream->impl.database_file_stream, buffer, mode, size);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return setvbuf(stream->impl.plain_file_stream, buffer, mode, size);
    }

    return 1;
}

// 0x530490
int tig_file_fprintf(TigFile* stream, const char* format, ...)
{
    int rc;
    va_list args;

    va_start(args, format);
    rc = tig_file_vfprintf(stream, format, args);
    va_end(args);

    return rc;
}

// 0x5304B0
int sub_5304B0()
{
    return -1;
}

// 0x5304C0
int tig_file_vfprintf(TigFile* stream, const char* format, va_list args)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_vfprintf(stream->impl.database_file_stream, format, args);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return vfprintf(stream->impl.plain_file_stream, format, args);
    }

    return -1;
}

// 0x530510
int tig_file_fgetc(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fgetc(stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fgetc(stream->impl.plain_file_stream);
    }

    return -1;
}

// 0x530540
char* tig_file_fgets(char* buffer, int max_count, TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fgets(buffer, max_count, stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fgets(buffer, max_count, stream->impl.plain_file_stream);
    }

    return NULL;
}

// 0x530580
int tig_file_fputc(int ch, TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fputc(ch, stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fputc(ch, stream->impl.plain_file_stream);
    }

    return -1;
}

// 0x5305C0
int tig_file_fputs(const char* str, TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fputs(str, stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fputs(str, stream->impl.plain_file_stream);
    }

    return -1;
}

// 0x530600
int tig_file_ungetc(int ch, TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_ungetc(ch, stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return ungetc(ch, stream->impl.plain_file_stream);
    }

    return -1;
}

// 0x530640
int tig_file_fread(void* buffer, size_t size, size_t count, TigFile* stream)
{
    if (size == 0 || count == 0) {
        return 0;
    }

    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fread(buffer, size, count, stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fread(buffer, size, count, stream->impl.plain_file_stream);
    }

    return count - 1;
}

// 0x5306A0
int tig_file_fwrite(const void* buffer, size_t size, size_t count, TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fwrite(buffer, size, count, stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fwrite(buffer, size, count, stream->impl.plain_file_stream);
    }

    return count - 1;
}

// 0x5306F0
int tig_file_fgetpos(TigFile* stream, int* pos_ptr)
{
    int pos;

    pos = tig_file_ftell(stream);
    if (pos == -1) {
        return 1;
    }

    *pos_ptr = pos;

    return 0;
}

// 0x530720
int tig_file_fseek(TigFile* stream, int offset, int origin)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_fseek(stream->impl.database_file_stream, offset, origin);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return fseek(stream->impl.plain_file_stream, offset, origin);
    }

    return 1;
}

// 0x530770
int tig_file_fsetpos(TigFile* stream, int* pos)
{
    return tig_file_fseek(stream, *pos, SEEK_SET);
}

// 0x530790
int tig_file_ftell(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_ftell(stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return ftell(stream->impl.plain_file_stream);
    }

    return -1;
}

// 0x5307C0
void tig_file_rewind(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        tig_database_rewind(stream->impl.database_file_stream);
    } else if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        rewind(stream->impl.plain_file_stream);
    }
}

// 0x5307F0
void tig_file_clearerr(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        tig_database_clearerr(stream->impl.database_file_stream);
    } else if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        clearerr(stream->impl.plain_file_stream);
    }
}

// 0x530820
int tig_file_feof(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_feof(stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return feof(stream->impl.plain_file_stream);
    }

    return 0;
}

// 0x530850
int tig_file_ferror(TigFile* stream)
{
    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        return tig_database_ferror(stream->impl.database_file_stream);
    }

    if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        return ferror(stream->impl.plain_file_stream);
    }

    return 0;
}

// 0x530880
void sub_530880(TigFileOutputFunc* error_func, TigFileOutputFunc* info_func)
{
    tig_database_set_pack_funcs(error_func, info_func);
}

// 0x5308A0
void sub_5308A0(int a1, int a2)
{
    (void)a1;
    (void)a2;
    // tig_database_pack(a1, a2);
}

// 0x5308C0
void sub_5308C0(int a1, int a2)
{
    (void)a1;
    (void)a2;
    // tig_database_unpack(a1, a2);
}

// 0x530B90
bool sub_530B90(const char* pattern)
{
    TigFileList list;
    int count;

    tig_file_list_create(&list, pattern);
    count = list.count;
    tig_file_list_destroy(&list);

    return count != 0;
}

// 0x530BD0
TigFile* tig_file_create()
{
    TigFile* stream;

    stream = (TigFile*)MALLOC(sizeof(*stream));
    stream->path = 0;
    stream->flags = 0;
    stream->impl.plain_file_stream = 0;

    return stream;
}

// 0x530BF0
void tig_file_destroy(TigFile* stream)
{
    FREE(stream);
}

// 0x530C00
bool tig_file_close_internal(TigFile* stream)
{
    bool success = 0;

    if ((stream->flags & TIG_FILE_DATABASE) != 0) {
        if (tig_database_ferror(stream->impl.database_file_stream) == 0) {
            success = true;
        }
    } else if ((stream->flags & TIG_FILE_PLAIN) != 0) {
        if (fclose(stream->impl.plain_file_stream) == 0) {
            success = true;
        }
    }

    if ((stream->flags & TIG_FILE_DELETE_ON_CLOSE) != 0) {
        tig_file_remove(stream->path);
        FREE(stream->path);
        stream->path = NULL;
        stream->flags &= ~TIG_FILE_DELETE_ON_CLOSE;
    }

    return success;
}

// 0x530C70
int tig_file_open_internal(const char* path, const char* mode, TigFile* stream)
{
    unsigned int ignored;
    TigFileRepository* repo;
    TigFileRepository* writeable_repo;
    TigDatabaseEntry* database_entry;
    char mutable_path[_MAX_PATH];

    stream->flags &= ~(TIG_FILE_DATABASE | TIG_FILE_PLAIN);

    if (path[0] == '.' || path[0] == '\\' || path[1] == ':') {
        stream->impl.plain_file_stream = fopen(path, mode);
        if (stream->impl.plain_file_stream == NULL) {
            return 0;
        }

        stream->flags |= TIG_FILE_PLAIN;
    } else {
        ignored = tig_file_ignored(path);

        repo = tig_file_repositories_head;
        while (repo != NULL) {
            if ((repo->type & TIG_FILE_REPOSITORY_DATABASE) != 0
                && (ignored & TIG_FILE_IGNORE_DATABASE) != 0
                && tig_database_get_entry(repo->database, path, &database_entry)) {
                if ((database_entry->flags & (TIG_DATABASE_ENTRY_0x100 | TIG_DATABASE_ENTRY_0x200)) != 0
                    && mode[0] == 'w') {
                    writeable_repo = tig_file_repositories_head;
                    while (writeable_repo != repo) {
                        if ((writeable_repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
                            strcpy(mutable_path, writeable_repo->path);
                            strcat(mutable_path, "\\");
                            strcat(mutable_path, path);

                            stream->impl.plain_file_stream = fopen(mutable_path, mode);
                            if (stream->impl.plain_file_stream != NULL) {
                                stream->flags |= TIG_FILE_PLAIN;
                                database_entry->flags &= ~TIG_DATABASE_ENTRY_0x100;
                                database_entry->flags |= TIG_DATABASE_ENTRY_0x200;
                                break;
                            }
                        }
                        writeable_repo = writeable_repo->next;
                    }
                }

                if ((stream->flags & TIG_FILE_PLAIN) == 0) {
                    database_entry->flags &= ~(TIG_DATABASE_ENTRY_0x100 | TIG_DATABASE_ENTRY_0x200);
                    stream->impl.database_file_stream = tig_database_fopen_entry(repo->database, database_entry, mode);
                    stream->flags |= TIG_FILE_DATABASE;
                }
                break;
            }
            repo = repo->next;
        }

        if ((stream->flags & (TIG_FILE_DATABASE | TIG_FILE_PLAIN)) == 0
            && (ignored & TIG_FILE_PLAIN) == 0) {
            repo = tig_file_repositories_head;
            while (repo != NULL) {
                if ((repo->type & TIG_FILE_REPOSITORY_DIRECTORY) != 0) {
                    strcpy(mutable_path, repo->path);
                    strcat(mutable_path, "\\");
                    strcat(mutable_path, path);

                    stream->impl.plain_file_stream = fopen(mutable_path, mode);
                    if (stream->impl.plain_file_stream != NULL) {
                        stream->flags |= TIG_FILE_PLAIN;
                        break;
                    }
                }
                repo = repo->next;
            }
        }
    }

    return stream->flags & (TIG_FILE_DATABASE | TIG_FILE_PLAIN);
}

// 0x530F90
void tig_file_process_attribs(unsigned int attribs, unsigned int* flags)
{
    *flags = 0;

    if ((attribs & _A_SUBDIR) != 0) {
        *flags |= TIG_FILE_ATTRIBUTE_SUBDIR;
    }

    if ((attribs & _A_RDONLY) != 0) {
        *flags |= TIG_FILE_ATTRIBUTE_READONLY;
    }

    if ((attribs & _A_HIDDEN) != 0) {
        *flags |= TIG_FILE_ATTRIBUTE_HIDDEN;
    }

    if ((attribs & _A_SYSTEM) != 0) {
        *flags |= TIG_FILE_ATTRIBUTE_SYSTEM;
    }

    if ((attribs & _A_ARCH) != 0) {
        *flags |= TIG_FILE_ATTRIBUTE_ARCHIVE;
    }
}

// 0x530FD0
void tig_file_list_add(TigFileList* list, TigFileInfo* info)
{
    if (list->count != 0) {
        int l = 0;
        int r = list->count - 1;
        int m;
        int cmp;

        do {
            m = (l + r) / 2;
            cmp = strcmpi(list->entries[m].path, info->path);
            if (cmp < 0) {
                l = m + 1;
            } else if (cmp > 0) {
                r = m - 1;
            } else {
                // Already added.
                return;
            }
        } while (l <= r);

        if (cmp < 0) {
            m += 1;
        }

        list->entries = (TigFileInfo*)REALLOC(list->entries, sizeof(TigFileInfo) * (list->count + 1));
        memcpy(&(list->entries[m + 1]), &(list->entries[m]), sizeof(TigFileInfo) * (list->count - m - 1));
        memcpy(&(list->entries[m]), info, sizeof(TigFileInfo));
        list->count++;
    } else {
        list->entries = (TigFileInfo*)MALLOC(sizeof(TigFileInfo));
        memcpy(list->entries, info, sizeof(TigFileInfo));
        list->count++;
    }
}

// 0x5310C0
void sub_5310C0()
{
    // TODO: Incomplete.
}

// 0x531170
unsigned int tig_file_ignored(const char* path)
{
    if (!fpattern_isvalid(path)) {
        return 0;
    }

    off_62B2A8 = tig_file_ignore_head;
    while (off_62B2A8 != NULL) {
        if (fpattern_matchn(off_62B2A8->path, path)) {
            return off_62B2A8->flags;
        }
        off_62B2A8 = off_62B2A8->next;
    }

    return 0;
}

// 0x5311D0
bool tig_file_copy(const char* src, const char* dst)
{
    TigFile* in;
    TigFile* out;

    if (!tig_file_exists(src, NULL)) {
        return false;
    }

    in = tig_file_fopen(src, "rb");
    if (in == NULL) {
        return false;
    }

    out = tig_file_fopen(dst, "wb");
    if (out == NULL) {
        tig_file_fclose(in);
        return false;
    }

    if (!tig_file_copy_internal(out, in)) {
        tig_file_fclose(out);
        tig_file_fclose(in);
        return false;
    }

    tig_file_fclose(out);
    tig_file_fclose(in);
    return true;
}

// 0x531250
bool tig_file_copy_internal(TigFile* dst, TigFile* src)
{
    unsigned char buffer[1 << 15];
    unsigned int size;

    size = tig_file_filelength(src);
    if (src == 0) {
        return false;
    }

    while (size >= sizeof(buffer)) {
        if (tig_file_fread(buffer, sizeof(buffer), 1, src) != 1) {
            return false;
        }

        if (tig_file_fwrite(buffer, sizeof(buffer), 1, dst) != 1) {
            return false;
        }

        size -= sizeof(buffer);
    }

    if (tig_file_fread(buffer, size, 1, src) != 1) {
        return false;
    }

    if (tig_file_fwrite(buffer, size, 1, dst) != 1) {
        return false;
    }

    return true;
}

// 0x531320
int tig_file_rmdir_recursively(const char* path)
{
    char mutable_path[_MAX_PATH];
    TigFindFileData ffd;

    strcpy(mutable_path, path);
    strcat(mutable_path, "\\*.*");

    if (tig_find_first_file(mutable_path, &ffd)) {
        strcpy(mutable_path, path);
        strcat(mutable_path, "\\");
        strcat(mutable_path, ffd.find_data.name);

        if ((ffd.find_data.attrib & _A_SUBDIR) != 0) {
            if (strcmp(ffd.find_data.name, ".") != 0
                && strcmp(ffd.find_data.name, "..") != 0) {
                tig_file_rmdir_recursively(mutable_path);
            }
        } else {
            remove(mutable_path);
        }
    }
    tig_find_close(&ffd);

    return rmdir(path);
}
