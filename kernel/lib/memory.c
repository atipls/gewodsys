#include "memory.h"

#include <stddef.h>

void RtZeroMemory(void *address, uint64_t size) {
    uint8_t *ptr = (uint8_t *) address;
    for (uint64_t i = 0; i < size; i++) {
        ptr[i] = 0;
    }
}

void RtCopyMemory(void *destination, const void *source, uint64_t size) {
    uint8_t *dest = (uint8_t *) destination;
    const uint8_t *src = (const uint8_t *) source;
    for (uint64_t i = 0; i < size; i++) {
        dest[i] = src[i];
    }
}

void RtFillMemory(void *address, uint64_t size, uint8_t value) {
    uint8_t *ptr = (uint8_t *) address;
    for (uint64_t i = 0; i < size; i++) {
        ptr[i] = value;
    }
}

// GNU Freestanding requires us to have these:
void *memcpy(void *dest, const void *source, size_t size) {
    RtCopyMemory(dest, source, size);
    return dest;
}


void *memmove(void *dest, const void *source, size_t size) {
    RtCopyMemory(dest, source, size);
    return dest;
}

void *memset(void * addr, int data, size_t length) {
    RtFillMemory(addr, length, data);
    return addr;
}

int memcmp(const void * a, const void * b, size_t size) {
    uint8_t *ptr1 = (uint8_t *) a;
    uint8_t *ptr2 = (uint8_t *) b;
    for (uint64_t i = 0; i < size; i++) {
        if (ptr1[i] != ptr2[i]) {
            return ptr1[i] - ptr2[i];
        }
    }
    return 0;
}
