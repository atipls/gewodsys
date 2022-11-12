#pragma once

#include <stdint.h>

typedef int8_t (*ArrayCompare)(void *, void *);

typedef struct OrderedArray {
    void **array;
    uint32_t size;
    uint32_t capacity;
    ArrayCompare predicate;
} OrderedArray;

int8_t PointerPredicate(void *a, void *b);

OrderedArray ArrayCreate(void *address, uint32_t capacity, ArrayCompare predicate);

void ArrayInsert(OrderedArray *array, void *item);
void *ArrayGet(OrderedArray *array, uint32_t index);
void ArrayRemove(OrderedArray *array, uint32_t index);
