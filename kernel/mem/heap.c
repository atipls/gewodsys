#include "heap.h"
#include "pmm.h"
#include "vmm.h"

#include <lib/array.h>
#include <lib/memory.h>
#include <utl/serial.h>

// Implementation adapted from JamesM's kernel development tutorials.

Heap *kHeap = 0;

static void HeapExpand(Heap *heap, uint64_t size) {
    if ((size & 0xFFFFF000) != 0) {
        size &= 0xFFFFF000;
        size += 0x1000;
    }

    uint64_t old_size = heap->end_address - heap->start_address;

    uint64_t offset = old_size;
    while (offset < size) {
        MmMapMemory((void *) (heap->start_address + offset), MmRequestPage());
        offset += PAGE_SIZE;
    }

    heap->end_address = heap->start_address + size;
}

static uint64_t HeapContract(Heap *heap, uint64_t size) {
    if (size & 0x1000) {
        size &= 0x1000;
        size += 0x1000;
    }

    if (size < HEAP_MIN_SIZE)
        size = HEAP_MIN_SIZE;

    uint64_t old_size = heap->end_address - heap->start_address;
    uint64_t offset = old_size - 0x1000;
    while (size < offset) {
        // free_frame(get_page(heap->start_address + i, 0, kernel_directory));
        offset -= 0x1000;
    }

    heap->end_address = heap->start_address + size;
    return size;
}

static int64_t HeapFindSmallestHole(Heap *heap, uint64_t size, int8_t page_align) {
    uint64_t iterator = 0;
    while (iterator < heap->index.size) {
        HeapHeader *header = (HeapHeader *) ArrayGet(&heap->index, iterator);
        if (page_align > 0) {
            uint64_t location = (uint64_t) header;
            int64_t offset = 0;
            if (((location + sizeof(HeapHeader)) & 0xFFFFF000) != 0)
                offset = PAGE_SIZE - (location + sizeof(HeapHeader)) % PAGE_SIZE;
            int64_t hole_size = (int64_t) header->size - offset;
            if (hole_size >= (int64_t) size)
                break;
        } else if (header->size >= size)
            break;
        iterator++;
    }

    if (iterator == heap->index.size)
        return -1;

    return iterator;
}

static int8_t HeaderPredicate(void *a, void *b) {
    return (((HeapHeader *) a)->size < ((HeapHeader *) b)->size) ? 1 : 0;
}

Heap *HeapCreate(uint64_t start, uint64_t end, uint64_t max, int8_t supervisor, int8_t readonly) {
    Heap *heap = MmGetIdentityPage();
    RtZeroMemory(heap, sizeof(Heap));

    heap->index = ArrayCreate((void *) start, HEAP_INDEX_SIZE, &HeaderPredicate);
    start += sizeof(void *) * HEAP_INDEX_SIZE;

    if ((start & 0xFFFFFFFFFFFFF000) != 0) {
        start &= 0xFFFFFFFFFFFFF000;
        start += 0x1000;
    }

    heap->start_address = start;
    heap->end_address = end;
    heap->max_address = max;
    heap->supervisor = supervisor;
    heap->readonly = readonly;

    HeapHeader *hole = (HeapHeader *) start;
    hole->size = end - start;
    hole->magic = HEAP_MAGIC;
    hole->is_hole = 1;
    ArrayInsert(&heap->index, (void *) hole);

    return heap;
}

void *HeapAllocate(Heap *heap, uint64_t size, int8_t page_align) {
    uint64_t new_size = size + sizeof(HeapHeader) + sizeof(HeapFooter);
    int64_t iterator = HeapFindSmallestHole(heap, new_size, page_align);

    if (iterator == -1) {
        uint64_t old_length = heap->end_address - heap->start_address;
        uint64_t old_end_address = heap->end_address;

        HeapExpand(heap, old_length + new_size);
        uint64_t new_length = heap->end_address - heap->start_address;

        iterator = 0;

        int64_t idx = -1;
        uint64_t value = 0x0;
        while (iterator < heap->index.size) {
            uint64_t tmp = (uint64_t) ArrayGet(&heap->index, iterator);
            if (tmp > value) {
                value = tmp;
                idx = iterator;
            }
            iterator++;
        }

        if (idx == -1) {
            HeapHeader *header = (HeapHeader *) old_end_address;
            header->magic = HEAP_MAGIC;
            header->size = new_length - old_length;
            header->is_hole = 1;
            HeapFooter *footer = (HeapFooter *) (old_end_address + header->size - sizeof(HeapFooter));
            footer->magic = HEAP_MAGIC;
            footer->header = header;
            ArrayInsert(&heap->index, (void *) header);
        } else {
            HeapHeader *header = ArrayGet(&heap->index, idx);
            header->size += new_length - old_length;
            HeapFooter *footer = (HeapFooter *) ((uint64_t) header + header->size - sizeof(HeapFooter));
            footer->header = header;
            footer->magic = HEAP_MAGIC;
        }

        return HeapAllocate(heap, size, page_align);
    }

    HeapHeader *orig_hole_header = (HeapHeader *) ArrayGet(&heap->index, iterator);
    uint64_t orig_hole_pos = (uint64_t) orig_hole_header;
    uint64_t orig_hole_size = orig_hole_header->size;

    if (orig_hole_size - new_size < sizeof(HeapHeader) + sizeof(HeapFooter)) {
        size += orig_hole_size - new_size;
        new_size = orig_hole_size;
    }

    if (page_align && orig_hole_pos & 0xFFFFF000) {
        uint64_t new_location = orig_hole_pos + PAGE_SIZE - (orig_hole_pos & 0xFFF) - sizeof(HeapHeader);
        HeapHeader *hole_header = (HeapHeader *) orig_hole_pos;
        hole_header->size = PAGE_SIZE - (orig_hole_pos & 0xFFF) - sizeof(HeapHeader);
        hole_header->magic = HEAP_MAGIC;
        hole_header->is_hole = 1;
        HeapFooter *hole_footer = (HeapFooter *) ((uint64_t) new_location - sizeof(HeapFooter));
        hole_footer->magic = HEAP_MAGIC;
        hole_footer->header = hole_header;
        orig_hole_pos = new_location;
        orig_hole_size = orig_hole_size - hole_header->size;
    } else {
        ArrayRemove(&heap->index, iterator);
    }


    HeapHeader *block_header = (HeapHeader *) orig_hole_pos;
    block_header->magic = HEAP_MAGIC;
    block_header->is_hole = 0;
    block_header->size = new_size;

    HeapFooter *block_footer = (HeapFooter *) (orig_hole_pos + sizeof(HeapHeader) + size);
    block_footer->magic = HEAP_MAGIC;
    block_footer->header = block_header;

    if (orig_hole_size - new_size > 0) {
        HeapHeader *hole_header = (HeapHeader *) (orig_hole_pos + sizeof(HeapHeader) + size + sizeof(HeapFooter));
        hole_header->magic = HEAP_MAGIC;
        hole_header->is_hole = 1;
        hole_header->size = orig_hole_size - new_size;
        HeapFooter *hole_footer = (HeapFooter *) ((uint64_t) hole_header + orig_hole_size - new_size - sizeof(HeapFooter));
        if ((uint64_t) hole_footer < heap->end_address) {
            hole_footer->magic = HEAP_MAGIC;
            hole_footer->header = hole_header;
        }
        ArrayInsert(&heap->index, (void *) hole_header);
    }

    return (void *) ((uint64_t) block_header + sizeof(HeapHeader));
}

void HeapFree(Heap *heap, void *address) {
    if (address == 0)
        return;

    HeapHeader *header = (HeapHeader *) ((uint64_t) address - sizeof(HeapHeader));
    HeapFooter *footer = (HeapFooter *) ((uint64_t) header + header->size - sizeof(HeapFooter));

    header->is_hole = 1;

    char do_add = 1;

    HeapFooter *test_footer = (HeapFooter *) ((uint64_t) header - sizeof(HeapFooter));
    if (test_footer->magic == HEAP_MAGIC &&
        test_footer->header->is_hole == 1) {
        uint64_t cache_size = header->size;
        header = test_footer->header;
        footer->header = header;
        header->size += cache_size;
        do_add = 0;
    }

    HeapHeader *test_header = (HeapHeader *) ((uint64_t) footer + sizeof(HeapFooter));
    if (test_header->magic == HEAP_MAGIC &&
        test_header->is_hole) {
        header->size += test_header->size;
        test_footer = (HeapFooter *) ((uint64_t) test_header + test_header->size - sizeof(HeapFooter));
        footer = test_footer;
        uint64_t iterator = 0;
        while ((iterator < heap->index.size) &&
               (ArrayGet(&heap->index, iterator) != (void *) test_header))
            iterator++;

        ArrayRemove(&heap->index, iterator);
    }

    if ((uint64_t) footer + sizeof(HeapFooter) == heap->end_address) {
        uint64_t old_length = heap->end_address - heap->start_address;
        uint64_t new_length = HeapContract(heap, (uint64_t) header - heap->start_address);
        if (header->size - (old_length - new_length) > 0) {
            header->size -= old_length - new_length;
            footer = (HeapFooter *) ((uint64_t) header + header->size - sizeof(HeapFooter));
            footer->magic = HEAP_MAGIC;
            footer->header = header;
        } else {
            uint64_t iterator = 0;
            while ((iterator < heap->index.size) &&
                   (ArrayGet(&heap->index, iterator) != (void *) test_header))
                iterator++;
            if (iterator < heap->index.size)
                ArrayRemove(&heap->index, iterator);
        }
    }

    // If required, add us to the index.
    if (do_add == 1)
        ArrayInsert(&heap->index, (void *) header);
}

void MmInitializeHeap(void) {
    for (uint64_t offset = 0; offset < HEAP_SIZE; offset += PAGE_SIZE)
        MmMapMemory((void *) (HEAP_ADDR + offset), MmRequestPage());

    kHeap = HeapCreate(HEAP_ADDR, HEAP_ADDR + HEAP_SIZE, HEAP_ADDR + 0x40000000, 0, 0);
}

void *kmalloc(uint32_t size) {
    return HeapAllocate(kHeap, size, 0);
}

void kfree(void *address) {
    HeapFree(kHeap, address);
}
