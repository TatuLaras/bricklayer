#include "path.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

char *path_get_corresponding_texture_file(const char *src) {
    size_t length = strlen(src);
    if (length < 3)
        return 0;
    size_t target_length = length + 6;

    char *destination = (char *)malloc(target_length);
    memcpy(destination, src, length);
    destination[target_length - 9] = 0;
    strcat(destination, "aseprite");

    return destination;
}
