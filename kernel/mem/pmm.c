#include "pmm.h"

#include <limine.h>
#include <lib/memory.h>
#include <utl/serial.h>

#define ALIGN_ADDR(x) (((PAGE_SIZE - 1) & (x)) ? ((x + PAGE_SIZE) & ~(PAGE_SIZE - 1)) : (x))
#define IS_ALIGNED(x) ((((uint64_t) (x)) & (PAGE_SIZE - 1)) == 0)

#define ADDR_TO_PAGE(x) ((uint64_t) (x) >> 12)

#define SET_PAGE_BIT(x) (kMemory->bitfield[(x) >> 3] |= (1 << ((x) % 8)))
#define CLEAR_PAGE_BIT(x) (kMemory->bitfield[(x) >> 3] &= ~(1 << ((x) % 8)))
#define GET_PAGE_BIT(x) ((kMemory->bitfield[(x) >> 3] & (1 << ((x) % 8))) >> ((x) % 8))

MemoryStatistics *kMemory;

static volatile struct limine_memmap_request memmap_request = {
        .id = LIMINE_MEMMAP_REQUEST,
        .revision = 0,
};

int MmInitialize() {
    uint64_t total_memory = 0, usable_memory = 0;

    struct limine_memmap_entry *largest;
    struct limine_memmap_response *memmap = memmap_request.response;
    for (uint64_t entry_index = 0; entry_index < memmap->entry_count; entry_index++) {
        struct limine_memmap_entry *entry = memmap->entries[entry_index];
        total_memory += entry->length;
        if (entry->type != LIMINE_MEMMAP_USABLE)
            continue;

        usable_memory += entry->length;

        if (!largest || entry->length > largest->length)
            largest = entry;
    }

    uint64_t first_available_address = ALIGN_ADDR(largest->base);
    uint64_t last_available_address = ALIGN_ADDR(largest->base + largest->length);
    uint64_t available_pages = (last_available_address - first_available_address) >> 12;
    uint64_t bitfield_entries = ((available_pages % 8) == 0) ? (available_pages >> 3) : (available_pages >> 3) + 1;

    if (largest->length < (bitfield_entries + sizeof(MemoryStatistics) + (3 << 12)))
        ComPrint("init_memory: not enough memory for memory bitfield");

    kMemory = (MemoryStatistics *) first_available_address;
    kMemory->bitfield = (uint8_t *) ALIGN_ADDR(first_available_address + sizeof(MemoryStatistics));

    RtZeroMemory(kMemory->bitfield, bitfield_entries);

    kMemory->total_available_pages = available_pages;
    kMemory->total_memory = total_memory;
    kMemory->total_pages = total_memory >> 12;

    kMemory->free_memory = last_available_address - first_available_address;
    kMemory->locked_memory = 0;
    kMemory->reserved_memory = 0;

    kMemory->last_available_page_address = last_available_address;
    kMemory->first_available_page_addr = first_available_address;

    if (!IS_ALIGNED(kMemory) || !IS_ALIGNED(kMemory->bitfield) || !IS_ALIGNED(kMemory->first_available_page_addr))
        ComPrint("Memory alignment error!");

    MmReservePages((void *) 0, (bitfield_entries + sizeof(MemoryStatistics) + (3 << 12)) >> 12);

    return 1;
}

void MmReservePage(void *address) {
    SET_PAGE_BIT(ADDR_TO_PAGE(address));

    kMemory->free_memory -= PAGE_SIZE;
    kMemory->reserved_memory += PAGE_SIZE;
}

void MmUnreservePage(void *address) {
    CLEAR_PAGE_BIT(ADDR_TO_PAGE(address));

    kMemory->free_memory += PAGE_SIZE;
    kMemory->reserved_memory -= PAGE_SIZE;
}

void MmLockPage(void *address) {
    SET_PAGE_BIT(ADDR_TO_PAGE(address));

    kMemory->free_memory -= PAGE_SIZE;
    kMemory->locked_memory += PAGE_SIZE;
}

void MmUnlockPage(void *address) {
    CLEAR_PAGE_BIT(ADDR_TO_PAGE(address));

    kMemory->free_memory += PAGE_SIZE;
    kMemory->locked_memory -= PAGE_SIZE;
}

void MmReservePages(void *address, uint64_t pages) {
    for (uint64_t i = 0; i < pages; i++)
        MmReservePage((void *) ((uint64_t) address + (i << 12)));
}

void MmUnreservePages(void *address, uint64_t pages) {
    for (uint64_t i = 0; i < pages; i++)
        MmUnreservePage((void *) ((uint64_t) address + (i << 12)));
}

void MmLockPages(void *address, uint64_t pages) {
    for (uint64_t i = 0; i < pages; i++)
        MmLockPage((void *) ((uint64_t) address + (i << 12)));
}

void MmUnlockPages(void *address, uint64_t pages) {
    for (uint64_t i = 0; i < pages; i++)
        MmUnlockPage((void *) ((uint64_t) address + (i << 12)));
}


void *MmRequestPage() {
    static uint64_t last_requested;
    uint64_t first = last_requested - 1;

    while (GET_PAGE_BIT(last_requested) == 1) {
        if (last_requested >= kMemory->total_available_pages)
            last_requested = 0;
        if (last_requested == first) {
            ComPrint("[MM] No free pages available!\n");
        }
        last_requested++;
    }

    MmLockPage((void *) (last_requested << 12));

    return (void *) (kMemory->first_available_page_addr + (last_requested << 12));
}

void MmFreePage(void *addr) {
    MmUnlockPage((void *) ((uint64_t)addr - kMemory->first_available_page_addr));
}
