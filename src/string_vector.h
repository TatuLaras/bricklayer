#ifndef _STRING_VECTOR
#define _STRING_VECTOR

#include <stddef.h>

typedef struct {
    char *string_data;
    size_t string_data_allocated;
    size_t string_data_used;
    size_t *string_indices;
    size_t string_indices_allocated;
    size_t string_indices_used;
} StringVector;

StringVector stringvec_init(void);
void stringvec_append(StringVector *vec, char *buffer, size_t buffer_size);
char *stringvec_get(StringVector *vec, size_t index);
void stringvec_free(StringVector *vec);
size_t stringvec_count(StringVector *vec);

#endif
