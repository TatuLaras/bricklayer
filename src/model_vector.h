#ifndef _MODEL_VECTOR
#define _MODEL_VECTOR

#include <raylib.h>
#include <stddef.h>

typedef struct {
    Model *data;
    size_t data_allocated;
    size_t data_used;
} ModelVector;

ModelVector modelvec_init(void);
void modelvec_append(ModelVector *vec, Model model);
Model *modelvec_get(ModelVector *vec, size_t index);
void modelvec_free(ModelVector *vec);

#endif
