#include "memory.h"


void RtZeroMemory(void *address, uint64_t size) {
    uint8_t *ptr = (uint8_t*)address;
    for (uint64_t i = 0; i < size; i++) {
        ptr[i] = 0;
    }
}

void RtCopyMemory(void *destination, void *source, uint64_t size) {
    uint8_t *dest = (uint8_t*)destination;
    uint8_t *src = (uint8_t*)source;
    for (uint64_t i = 0; i < size; i++) {
        dest[i] = src[i];
    }
}

void RtFillMemory(void *address, uint64_t size, uint8_t value) {
    uint8_t *ptr = (uint8_t*)address;
    for (uint64_t i = 0; i < size; i++) {
        ptr[i] = value;
    }
}