#ifndef TIG_FILE_CACHE_H_
#define TIG_FILE_CACHE_H_

// FILECACHE
//
// The FILECACHE subsystem provides `TigFileCache` object used to cache files
// loaded from FILE module.
//
// Implements a least-recently-used cache. The maximum number of files and
// maximum the cache can hold is provided during creation.
//
// NOTES
//
// - Unlike many other subsystems which track allocations, file cache objects
// are not tracked at subsystem level. Which in turn means `tig_file_cache_exit`
// does not release existing cache objects. You're responsible for releasing of
// file cache objects with `tig_file_cache_destroy` before calling
// `tig_file_cache_exit`.
//
// - The `TigFileCache` resembles `Cache` object from Fallouts, but it`s API
// and implementation is much simpler.
//
// - There is only one use case in both Arcanum and ToEE - caching sound files
// (see SOUND subsystem). This is different from Fallouts where `Cache` was also
// used for art files. In TIG the ART subsystem has it's own cache, which is
// considered implementation detail and have no public API.

#include <time.h>

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Represents cached file.
typedef struct TigFileCacheEntry {
    void* data;
    int size;
    int index;
    char* path;
} TigFileCacheEntry;

static_assert(sizeof(TigFileCacheEntry) == 0x10, "wrong size");

// An item in file cache.
typedef struct TigFileCacheItem {
    TigFileCacheEntry entry;
    int refcount;
    time_t timestamp;
} TigFileCacheItem;

static_assert(sizeof(TigFileCacheItem) == 0x18, "wrong size");

// A collection of cached files.
typedef struct TigFileCache {
    int signature;
    int capacity;
    int max_size;
    int bytes;
    int items_count;
    TigFileCacheItem* items;
} TigFileCache;

static_assert(sizeof(TigFileCache) == 0x18, "wrong size");

// Initializes file cache system.
int tig_file_cache_init(TigInitializeInfo* init_info);

// Shutdowns file cache system.
void tig_file_cache_exit();

// Evicts ununsed entries from cache.
void tig_file_cache_flush(TigFileCache* cache);

// Creates a new file cache.
//
// - `capacity`: total nubmer of files this cache object can manage.
// - `max_size`: max size of files this cache object can manage.
TigFileCache* tig_file_cache_create(int capacity, int max_size);

// Destroys the given file cache.
//
// NOTE: It's an error to have acquired but not released entries, which is a
// memory leak, but this is neither checked, nor enforced.
void tig_file_cache_destroy(TigFileCache* cache);

// Fetches file with given path from cache loading it from the file system if
// needed.
TigFileCacheEntry* tig_file_cache_acquire(TigFileCache* cache, const char* path);

// Releases access to given entry.
void tig_file_cache_release(TigFileCache* cache, TigFileCacheEntry* entry);

#ifdef __cplusplus
}
#endif

#endif /* TIG_FILE_CACHE_H_ */
