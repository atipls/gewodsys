#pragma once
#include <stdint.h>
#include <lib/array.h>

#define HEAP_ADDR 0x0000100000000000
#define HEAP_SIZE PAGE_SIZE * 65536
#define HEAP_MIN_SIZE 0x70000
#define HEAP_INDEX_SIZE 0x20000
#define HEAP_MAGIC 0x123890AB

typedef struct HeapHeader {
    uint32_t magic;
    uint8_t is_hole;
    uint64_t size;
} HeapHeader;

typedef struct HeapFooter {
    uint32_t magic;
    HeapHeader *header;
} HeapFooter;

typedef struct Heap {
    OrderedArray index;
    uint64_t start_address;
    uint64_t end_address;
    uint64_t max_address;
    uint8_t supervisor;
    uint8_t readonly;
} Heap;

Heap *HeapCreate(uint64_t start, uint64_t end, uint64_t max, int8_t supervisor, int8_t readonly);
void *HeapAllocate(Heap *heap, uint64_t size, int8_t page_align);
void HeapFree(Heap *heap, void *p);

void MmInitializeHeap(void);

void *kmalloc(uint32_t size);
void kfree(void *address);
