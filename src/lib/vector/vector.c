#include <stdio.h>
#include <stdlib.h>

#include "vector.h"

void vector_init(vector *v, size_t capacity) {
    v->capacity = capacity > 0 || VECTOR_INIT_CAPACITY;
    v->total = 0;
    v->items = malloc(sizeof(void *) * v->capacity);
}

size_t vector_total(vector *v) {
    return v->total;
}

static void vector_resize(vector *v, size_t capacity) {
    #ifdef DEBUG_ON
    printf("vector_resize: %d to %d\n", v->capacity, capacity);
    #endif

    void **items = realloc(v->items, sizeof(void *) * capacity);
    if (items) {
        v->items = items;
        v->capacity = capacity;
    }
}

void vector_add(vector *v, void *item) {
    if (v->capacity == v->total)
        vector_resize(v, v->capacity * VECTOR_CAPACITY_RESIZE);
    v->items[v->total++] = item;
}

void vector_set(vector *v, uint index, void *item) {
    if (index < v->total)
        v->items[index] = item;
}

void *vector_get(vector *v, uint index) {
    if (index < v->total)
        return v->items[index];
    return NULL;
}

void vector_delete(vector *v, uint index) {
    if (index >= v->total)
        return;

    v->items[index] = NULL;

    for (size_t i = index; i < v->total - 1; i++) {
        v->items[i] = v->items[i + 1];
    }

    v->items[ --v->total ] = NULL;

    if (v->total > 0 && v->total == v->capacity / 4)
        vector_resize(v, v->capacity / 2);
}

void vector_free(vector *v) {
    free(v->items);
}
