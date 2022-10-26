#pragma once

#include <stdint.h>

typedef struct Bitmap {
    uint64_t size;
    uint64_t *map;
} Bitmap;

uint8_t BmGet(Bitmap *bitmap, uint64_t index);
void BmSet(Bitmap *bitmap, uint64_t index);
void BmClear(Bitmap *bitmap, uint64_t index);

uint64_t BmFindFirstFree(Bitmap *bitmap);
uint64_t BmFindFirstSet(Bitmap *bitmap);

void BmPrint(Bitmap *bitmap);
