#include "array.h"
#include <lib/memory.h>

int8_t PointerPredicate(void *a, void *b) {
    return (a < b) ? 1 : 0;
}

OrderedArray ArrayCreate(void *address, uint32_t capacity, ArrayCompare predicate) {
    OrderedArray array;
    array.array = (void **) address;
    RtZeroMemory(array.array, capacity * sizeof(void *));

    array.size = 0;
    array.capacity = capacity;
    array.predicate = predicate;

    return array;
}

void ArrayInsert(OrderedArray *array, void *item) {
    uint32_t iterator = 0;
    while (iterator < array->size && array->predicate(array->array[iterator], item))
        iterator++;
    if (iterator == array->size) {
        array->array[array->size++] = item;
        return;
    }
    void *tmp = array->array[iterator];
    array->array[iterator] = item;
    while (iterator < array->size) {
        iterator++;
        void *tmp2 = array->array[iterator];
        array->array[iterator] = tmp;
        tmp = tmp2;
    }
    array->size++;
}

void *ArrayGet(OrderedArray *array, uint32_t index) {
    return array->array[index];
}

void ArrayRemove(OrderedArray *array, uint32_t index) {
    while (index < array->size) {
        array->array[index] = array->array[index + 1];
        index++;
    }
    array->size--;
}
