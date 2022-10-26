#include <stddef.h>
#include <stdint.h>

#include "memory.h"

#include <lib/bitmap.h>
#include <utl/serial.h>

typedef struct MemoryStatistics {
    uint64_t total_memory;
    uint64_t free_memory;
    uint64_t used_memory;
    uint64_t reserved_memory;
} MemoryStatistics;

static MemoryStatistics kMemoryStatistics;

static Bitmap kMemoryBitmap;

void MmZeroMemory(void *address, uint64_t size) {
    uint64_t *ptr64 = (uint64_t *) address;
    while (size >= 8) {
        *ptr64 = 0;
        ptr64++;
        size -= 8;
    }

    uint32_t *ptr32 = (uint32_t *) ptr64;
    while (size >= 4) {
        *ptr32 = 0;
        ptr32++;
        size -= 4;
    }

    uint16_t *ptr16 = (uint16_t *) ptr32;
    while (size >= 2) {
        *ptr16 = 0;
        ptr16++;
        size -= 2;
    }

    uint8_t *ptr8 = (uint8_t *) ptr16;
    while (size >= 1) {
        *ptr8 = 0;
        ptr8++;
        size -= 1;
    }
}

void MmCopyMemory(void *destination, void *source, uint64_t size) {
    uint64_t *ptr64 = (uint64_t *) destination;
    uint64_t *ptr64_source = (uint64_t *) source;
    while (size >= 8) {
        *ptr64 = *ptr64_source;
        ptr64++;
        ptr64_source++;
        size -= 8;
    }

    uint32_t *ptr32 = (uint32_t *) ptr64;
    uint32_t *ptr32_source = (uint32_t *) ptr64_source;
    while (size >= 4) {
        *ptr32 = *ptr32_source;
        ptr32++;
        ptr32_source++;
        size -= 4;
    }

    uint16_t *ptr16 = (uint16_t *) ptr32;
    uint16_t *ptr16_source = (uint16_t *) ptr32_source;
    while (size >= 2) {
        *ptr16 = *ptr16_source;
        ptr16++;
        ptr16_source++;
        size -= 2;
    }

    uint8_t *ptr8 = (uint8_t *) ptr16;
    uint8_t *ptr8_source = (uint8_t *) ptr16_source;
    while (size >= 1) {
        *ptr8 = *ptr8_source;
        ptr8++;
        ptr8_source++;
        size -= 1;
    }
}

void MmInitialize(struct limine_memmap_response *memmap) {
    static const char *const LIMINE_MEMORY_TYPES[] = {
            "USABLE",
            "RESERVED",
            "ACPI RECLAIMABLE",
            "ACPI NVS",
            "BAD MEMORY",
            "BOOTLOADER RECLAIMABLE",
            "KERNEL AND MODULES",
            "FRAMEBUFFER",
    };

    ComPrint("[MM]: Initializing with %d mmap entries.\n", memmap->entry_count);

    struct limine_memmap_entry *largest = NULL;
    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        ComPrint("[MM]: MMAP: 0x%X - 0x%X  type %s\n", entry->base,
                 entry->base + entry->length,
                 LIMINE_MEMORY_TYPES[entry->type]);
        if (entry->type == LIMINE_MEMMAP_USABLE && (!largest || entry->length > largest->length))
            largest = entry;

        if (entry->type == LIMINE_MEMMAP_USABLE)
            kMemoryStatistics.total_memory += entry->length;
    }

    kMemoryStatistics.free_memory = kMemoryStatistics.total_memory;

    // Initialize the kernel memory map
    kMemoryBitmap.size = largest->length / PAGE_SIZE + 1;
    kMemoryBitmap.map = (uint64_t *) largest->base;

    MmLockPage(0); // Lock the zero page

    MmLockPages(largest->base, largest->length / PAGE_SIZE + 1);

    for (size_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE)
            MmReservePages(entry->base, entry->length / PAGE_SIZE + 1);
    }
}

void MmLockPage(uint64_t address) {
    if (!BmGet(&kMemoryBitmap, address / PAGE_SIZE)) {
        kMemoryStatistics.free_memory -= PAGE_SIZE;
        kMemoryStatistics.used_memory += PAGE_SIZE;
    }
    BmSet(&kMemoryBitmap, address / PAGE_SIZE);
}


void MmUnlockPage(uint64_t address) {
    if (BmGet(&kMemoryBitmap, address / PAGE_SIZE)) {
        kMemoryStatistics.used_memory -= PAGE_SIZE;
        kMemoryStatistics.free_memory += PAGE_SIZE;
    }

    BmClear(&kMemoryBitmap, address / PAGE_SIZE);
}

void MmLockPages(uint64_t address, uint64_t size) {
    for (uint64_t i = 0; i < size; i += PAGE_SIZE)
        MmLockPage(address + i);
}

void MmUnlockPages(uint64_t address, uint64_t size) {
    for (uint64_t i = 0; i < size; i += PAGE_SIZE)
        MmUnlockPage(address + i);
}

void MmReservePage(uint64_t address) {
    if (!BmGet(&kMemoryBitmap, address / PAGE_SIZE)) {
        kMemoryStatistics.free_memory -= PAGE_SIZE;
        kMemoryStatistics.reserved_memory += PAGE_SIZE;
    }
    BmSet(&kMemoryBitmap, address / PAGE_SIZE);
}

void MmUnreservePage(uint64_t address) {
    if (BmGet(&kMemoryBitmap, address / PAGE_SIZE)) {
        kMemoryStatistics.reserved_memory -= PAGE_SIZE;
        kMemoryStatistics.free_memory += PAGE_SIZE;
    }

    BmClear(&kMemoryBitmap, address / PAGE_SIZE);
}

void MmReservePages(uint64_t address, uint64_t size) {
    for (uint64_t i = 0; i < size; i += PAGE_SIZE)
        MmReservePage(address + i);
}

void MmUnreservePages(uint64_t address, uint64_t size) {
    for (uint64_t i = 0; i < size; i += PAGE_SIZE)
        MmUnreservePage(address + i);
}

void *MmAllocate(uint64_t size) {
    uint64_t pages = size / PAGE_SIZE + 1;
    uint64_t index = BmFindFirstFree(&kMemoryBitmap);
    if (index == kMemoryBitmap.size) {
        ComPrint("[MM]: Out of memory! (%d, %d)\n", kMemoryBitmap.size, index);
        return NULL;
    }

    MmLockPages(index * PAGE_SIZE, pages);

    return (void *) (index * PAGE_SIZE);
}

void MmFree(void *address, uint64_t size) {
    uint64_t pages = size / PAGE_SIZE + 1;
    MmUnlockPages((uint64_t) address, pages);
}

uint64_t MmGetTotalMemory() {
    return kMemoryStatistics.total_memory;
}

uint64_t MmGetFreeMemory() {
    return kMemoryStatistics.free_memory;
}

uint64_t MmGetUsedMemory() {
    return kMemoryStatistics.used_memory;
}

uint64_t MmGetReservedMemory() {
    return kMemoryStatistics.reserved_memory;
}