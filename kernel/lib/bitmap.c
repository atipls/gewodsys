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
    for (uint64_t i = 0; i < bitmap->size; i++)
        if (!BmGet(bitmap, i))
            return i;

    return bitmap->size;
}

uint64_t BmFindFirstSet(Bitmap *bitmap) {
    for (uint64_t i = 0; i < bitmap->size; i++)
        if (BmGet(bitmap, i))
            return i;

    return bitmap->size;
}

void BmPrint(Bitmap *bitmap) {
    for (uint64_t i = 0; i < bitmap->size; i++) {
        if (BmGet(bitmap, i))
            ComPutChar('1');
        else
            ComPutChar('0');
    }
}