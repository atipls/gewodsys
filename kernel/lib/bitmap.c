#include "bitmap.h"
#include <utl/serial.h>

uint8_t BmGet(Bitmap *bitmap, uint64_t index) {
    return (bitmap->map[index / 64] >> (index % 64)) & 1;
}

void BmSet(Bitmap *bitmap, uint64_t index) {
    bitmap->map[index / 64] |= 1 << (index % 64);
}

void BmClear(Bitmap *bitmap, uint64_t index) {
    bitmap->map[index / 64] &= ~(1 << (index % 64));
}

uint64_t BmFindFirstFree(Bitmap *bitmap) {
    for (uint64_t i = 0; i < bitmap->size; i++) {
        if (bitmap->map[i] != 0xFFFFFFFFFFFFFFFF) {
            for (uint64_t j = 0; j < 64; j++) {
                if (!(bitmap->map[i] & (1 << j)))
                    return i * 64 + j;
            }
        }
    }

    return bitmap->size;
}

uint64_t BmFindFirstSet(Bitmap *bitmap) {
    for (uint64_t i = 0; i < bitmap->size; i++) {
        if (bitmap->map[i] != 0) {
            for (uint64_t j = 0; j < 64; j++) {
                if (bitmap->map[i] & (1 << j))
                    return i * 64 + j;
            }
        }
    }

    return 0;
}