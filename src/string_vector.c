#include "string_vector.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define STARTING_SIZE_CHARACTERS 200
#define STARTING_SIZE_INDICES 4
#define GROWTH_FACTOR 2

StringVector stringvec_init(void) {
    StringVector vec = {
        .string_data = malloc(STARTING_SIZE_CHARACTERS),
        .string_data_allocated = STARTING_SIZE_CHARACTERS,
        .string_indices = malloc(STARTING_SIZE_INDICES * sizeof(size_t)),
        .string_indices_allocated = STARTING_SIZE_INDICES,
    };
    return vec;
}

void stringvec_append(StringVector *vec, char *buffer, size_t buffer_size) {
    while (vec->string_data_used + buffer_size >= vec->string_data_allocated) {
        // Grow char buffer
        vec->string_data_allocated *= GROWTH_FACTOR;
        vec->string_data =
            realloc(vec->string_data, vec->string_data_allocated);
        assert(vec->string_data);
    }
    if (vec->string_indices_used >= vec->string_indices_allocated) {
        // Grow pointer buffer
        vec->string_indices_allocated *= GROWTH_FACTOR;
        vec->string_indices =
            realloc(vec->string_indices,
                    vec->string_indices_allocated * sizeof(char *));
        assert(vec->string_indices);
    }

    size_t index = vec->string_data_used;
    char *pointer = vec->string_data + index;
    memcpy(pointer, buffer, buffer_size);
    vec->string_data_used += buffer_size;

    // Extra null terminator to be safe
    vec->string_data[vec->string_data_used++] = 0;

    vec->string_indices[vec->string_indices_used++] = index;
}

char *stringvec_get(StringVector *vec, size_t index) {
    if (index >= vec->string_indices_used)
        return 0;

    return vec->string_data + vec->string_indices[index];
}

void stringvec_free(StringVector *vec) {
    if (vec->string_data) {
        free(vec->string_data);
        vec->string_data = 0;
    }
    if (vec->string_indices) {
        free(vec->string_indices);
        vec->string_indices = 0;
    }
}

size_t stringvec_count(StringVector *vec) {
    return vec->string_indices_used;
}
