#ifndef FIREWATCH_MAX_DIRECTORIES
#define FIREWATCH_MAX_DIRECTORIES 32
#endif

#ifndef _FIREWATCH
#define _FIREWATCH

/*
   firewatch - A single-header library to simplify the implementation of
   hot-reload capablilities.

   This is a single-header library. Similar to other libraries of this kind, you
   need to define FIREWATCH_IMPLEMENTATION (followed by inclusion of this file)
   in exactly one source file.

   #define FIREWATCH_IMPLEMENTATION
   #include "firewatch.h"

   See README.md for usage instructions and examples.

   Depends on inotify and some standard library functions. Exact dependencies
   are listed as includes below this comment.
*/

#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

#define PATH_MAX 4096

typedef void (*FileRefreshFunction)(const char *file_path, uint64_t cookie);

typedef struct {
    char filepath[PATH_MAX];
    uint32_t kind;
    uint64_t cookie;
} LoadRequest;

typedef struct {
    LoadRequest request;
    size_t filename_offset;
    FileRefreshFunction on_change_callback;
    // True if updates to this file should be pushed to the updated files stack
    // instead of calling the on_change_callback.
    int using_stack;
} FileInfo;

typedef struct {
    FileInfo *data;
    size_t data_allocated;
    size_t data_used;
} FileInfoVector;

// Registers a new file at `filepath` to be watched by firewatch.
//
// `filepath`: The file to watch. Must be a file, not a directory. The parent
// directory of the file must exist, the file itself doesn't need to.
// Relative paths allowed.
//
// `on_change_callback`: A fuction pointer for a function that takes a single
// const char pointer as argument that will be equal to `filepath` when
// firewatch calls it.
//
// If `on_change_callback` is NULL, then updates to the file will be pushed to
// an internal stack, which can be accessed using firewatch_request_stack_pop.
//
// `cookie`: Any arbitrary integer that will be passed to the callback as an
// argument (or returned as part of a LoadRequest if using the stack).
void firewatch_new_file(const char *filepath,
                        FileRefreshFunction on_change_callback,
                        uint64_t cookie);

// A version of firewatch_new_file with extended option `kind`, which will be
// some arbitrary integer that will be returned as part of a LoadRequest when
// using the stack method instead of the callback method.
// You can use it to differentiate between loading different kinds of files,
// something which is was previously achieved through different callbacks with
// the callback method.
void firewatch_new_file_ex(const char *filepath,
                           FileRefreshFunction on_change_callback,
                           uint64_t cookie, uint32_t kind);

// Pops one LoadRequest from the stack of newly updated files in need of a
// reload and writes it into `out_load_request`. Returns 1 if a request was
// successfully popped, 0 if there is no requests in the stack.
int firewatch_request_stack_pop(LoadRequest *out_load_request);

#endif // _FIREWATCH
#ifdef FIREWATCH_IMPLEMENTATION

#define _BUF_SIZE (1024 * (sizeof(struct inotify_event) + 16))

// --- Dynamic array for file info structs ---
FileInfoVector fileinfovec_init(void) {
    FileInfoVector vec = {
        .data = malloc(4 * sizeof(FileInfo)),
        .data_allocated = 4,
    };
    return vec;
}

size_t fileinfovec_append(FileInfoVector *vec, FileInfo data) {
    if (vec->data_used >= vec->data_allocated) {
        vec->data_allocated *= 2;
        vec->data = realloc(vec->data, vec->data_allocated * sizeof(FileInfo));
        if (!vec->data)
            abort();
    }
    vec->data[vec->data_used++] = data;
    return vec->data_used - 1;
}

FileInfo *fileinfovec_get(FileInfoVector *vec, size_t index) {
    if (index >= vec->data_used)
        return 0;
    return vec->data + index;
}

void fileinfovec_free(FileInfoVector *vec) {
    if (vec->data) {
        free(vec->data);
        vec->data = 0;
    }
}

// --- Firewatch implementation starts ---

static FileInfoVector fw_file_info_lists[FIREWATCH_MAX_DIRECTORIES];

static int fw_inotify_fp = -1;
static pthread_t fw_thread_id = 0;
static pthread_mutex_t fw_lock;
static FileInfoVector fw_needs_refresh_queue = {0};

// Last occurrence of character '/' in `string` plus one.
// Returns 0 if no slashes in `string`.
static inline int fw_basename_start_index(const char *string) {
    size_t last = 0;
    size_t i = 0;
    while (string[i]) {
        if (string[i] == '/')
            last = i + 1;
        i++;
    }

    return last;
}

// Will run in a thread and read inotify events, calling the callback if
// necessary.
static void *fw_watch_for_changes(void *_a) {
    assert(fw_inotify_fp > 0);
    char buf[_BUF_SIZE] = {0};
    size_t size, i = 0;

    while (1) {
        size = read(fw_inotify_fp, buf, _BUF_SIZE);
        i = 0;
        while (i < size) {
            struct inotify_event *event = (struct inotify_event *)buf + i;
            i += sizeof(struct inotify_event) + event->len;
            if (!event->mask || !event->len || event->wd <= 0)
                continue;
            if (!fw_file_info_lists[event->wd].data)
                continue;

            pthread_mutex_lock(&fw_lock);

            for (size_t j = 0; j < fw_file_info_lists[event->wd].data_used;
                 j++) {
                FileInfo *file_info =
                    fileinfovec_get(fw_file_info_lists + event->wd, j);

                if (strcmp(file_info->request.filepath +
                               file_info->filename_offset,
                           event->name))
                    continue;

                // Two methods, stack and callback
                if (file_info->using_stack) {
                    fileinfovec_append(&fw_needs_refresh_queue, *file_info);
                } else {
                    (file_info->on_change_callback)(file_info->request.filepath,
                                                    file_info->request.cookie);
                }
            }

            pthread_mutex_unlock(&fw_lock);
        }
    }

    return 0;
}

// Ensure all necassary resources have been initialized.
static inline void fw_ensure_init(void) {
    if (fw_inotify_fp <= 0)
        fw_inotify_fp = inotify_init();

    // Create watch thread
    if (!fw_thread_id) {
        pthread_create(&fw_thread_id, NULL, &fw_watch_for_changes, 0);
        assert(fw_thread_id);
        pthread_detach(fw_thread_id);
    }

    if (!fw_needs_refresh_queue.data)
        fw_needs_refresh_queue = fileinfovec_init();

    assert((fw_inotify_fp > 0));
}

void firewatch_new_file(const char *filepath,
                        FileRefreshFunction on_change_callback,
                        uint64_t cookie) {
    firewatch_new_file_ex(filepath, on_change_callback, cookie, 0);
}

//  TODO: Return handle, removable watch
void firewatch_new_file_ex(const char *filepath,
                           FileRefreshFunction on_change_callback,
                           uint64_t cookie, uint32_t kind) {
    fw_ensure_init();

    FileInfo file_info = {.on_change_callback = on_change_callback,
                          .using_stack = on_change_callback == 0,
                          .request = {.cookie = cookie, .kind = kind}};

    strncpy(file_info.request.filepath, filepath, PATH_MAX);

    size_t filename_start = fw_basename_start_index(filepath);
    file_info.filename_offset = filename_start;

    char directory[PATH_MAX] = {0};
    if (filename_start == 0)
        directory[0] = '.';
    else
        memcpy(directory, filepath, filename_start);

    // Create watch
    int wd = inotify_add_watch(fw_inotify_fp, directory,
                               IN_CLOSE_WRITE | IN_MOVED_TO);
    if (wd <= 0) {
        fprintf(
            stderr,
            "ERROR: could not begin watching file changes for file %s, maybe "
            "the parent directory of the file does not exist?\n",
            filepath);
        return;
    }

    pthread_mutex_lock(&fw_lock);

    if (!fw_file_info_lists[wd].data)
        fw_file_info_lists[wd] = fileinfovec_init();
    fileinfovec_append(fw_file_info_lists + wd, file_info);

    pthread_mutex_unlock(&fw_lock);

    if (!on_change_callback)
        fileinfovec_append(&fw_needs_refresh_queue, file_info);
    else
        (on_change_callback)(filepath, cookie);
}

int firewatch_request_stack_pop(LoadRequest *out_load_request) {
    pthread_mutex_lock(&fw_lock);

    if (fw_needs_refresh_queue.data_used == 0) {
        pthread_mutex_unlock(&fw_lock);
        return 0;
    }

    *out_load_request =
        fw_needs_refresh_queue.data[--fw_needs_refresh_queue.data_used].request;

    pthread_mutex_unlock(&fw_lock);
    return 1;
}

#endif // FIREWATCH_IMPLEMENTATION
