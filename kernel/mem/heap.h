#pragma once
#include <stdint.h>

#define HEAP_ADDR 0x0000100000000000
#define HEAP_SIZE PAGE_SIZE * 65536

void MmInitializeHeap();

void *HeapAllocate(uint32_t size);
void HeapFree(void * address);
