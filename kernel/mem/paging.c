#include "paging.h"
#include "memory.h"
#include <utl/serial.h>

static uint64_t MmGetPi(uint64_t virtual_address) { return (virtual_address >> 12) & 0x1FF; }
static uint64_t MmGetPt(uint64_t virtual_address) { return (virtual_address >> 21) & 0x1FF; }
static uint64_t MmGetPd(uint64_t virtual_address) { return (virtual_address >> 30) & 0x1FF; }
static uint64_t MmGetPdp(uint64_t virtual_address) { return (virtual_address >> 39) & 0x1FF; }

static PageTable *kPML4;

static volatile struct limine_kernel_address_request kernel_address_request = {
        .id = LIMINE_KERNEL_ADDRESS_REQUEST,
        .revision = 1,
};

extern uint64_t kTextStart, kTextEnd;
extern uint64_t kRodataStart, kRodataEnd;
extern uint64_t kDataStart, kDataEnd;

extern uint64_t kKernelSize;

void MmInitializePaging(struct limine_memmap_response *memmap) {
    kPML4 = MmAllocate(sizeof(PageTable));
    MmZeroMemory(kPML4, sizeof(PageTable));

    struct limine_kernel_address_response *kernel_address = kernel_address_request.response;

    ComPrint("Kernel text: 0x%X - 0x%X\n", &kTextStart, &kTextEnd);
    ComPrint("Kernel rodata: 0x%X - 0x%X\n", &kRodataStart, &kRodataEnd);
    ComPrint("Kernel data: 0x%X - 0x%X\n", &kDataStart, &kDataEnd);
    ComPrint("Kernel size: 0x%X\n", &kKernelSize);

    ComPrint("Kernel address: 0x%X\n", kernel_address->physical_base);

    for (uint64_t entry_index = 0; entry_index < memmap->entry_count; entry_index++) {
        const struct limine_memmap_entry *entry = memmap->entries[entry_index];
        uintptr_t address = entry->base;
        if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_FRAMEBUFFER) {
            for (uint64_t offset = 0; offset < entry->length; offset += 0x1000)
                MmMapPage(address + offset + 0xFFFFFFFF80000000, address + offset, kPagePresent | kPageReadWrite);
        }
    }

    for (uint64_t i = 0; i < 0x100000000; i += 0x1000) {
        MmMapPage(0xFFFFFFFF80000000 + i, i, kPagePresent | kPageReadWrite);
    }

    for (uint64_t i = 0; i < 0x10000; i += 0x1000) {
        MmMapPage(
                kernel_address->virtual_base + i,
                kernel_address->physical_base + i,
                kPagePresent | kPageReadWrite);
    }


    ComPrint("[MM]: PML4: 0x%X, enabling paging.\n", kPML4);

    __asm__ volatile("mov %%cr3, %0" ::"r"(kPML4)
                     : "memory");

    ComPrint("[MM]: Paging initialized.\n");
}

uint64_t MmMapToVirtual(uint64_t physical_address, uint64_t size) {
    uint64_t virtual_address = (uint64_t) MmAllocate(size);
    uint64_t virtual_address_start = virtual_address;
    while (virtual_address < virtual_address_start + size) {
        MmMapPage(virtual_address, physical_address, kPageDisableCache | kPageWriteThrough);
        virtual_address += 0x1000;
        physical_address += 0x1000;
    }
    return virtual_address_start;
}

uint64_t MmGetPhysicalAddress(uint64_t virtual_address) {
    PageTable *pdp = (PageTable *) (uintptr_t) kPML4->pde[MmGetPdp(virtual_address)].address;
    PageTable *pd = (PageTable *) (uintptr_t) pdp->pde[MmGetPd(virtual_address)].address;
    PageTable *pt = (PageTable *) (uintptr_t) pd->pde[MmGetPt(virtual_address)].address;
    return pt->pde[MmGetPi(virtual_address)].address;
}

void MmUnmapVirtual(uint64_t virtual_address, uint64_t size) {
    (void) virtual_address;
    (void) size;
}

void MmMapPage(uint64_t virtual_address, uint64_t physical_address, uint64_t flags) {
    ComPrint("MmMapPage(0x%X, 0x%X, 0x%X); // Pdp %d Pd %d Pt %d Pi %d\n", virtual_address, physical_address, flags, MmGetPdp(virtual_address), MmGetPd(virtual_address), MmGetPt(virtual_address), MmGetPi(virtual_address));

    PageTable *pdp = (PageTable *) (uintptr_t) kPML4->pde[MmGetPdp(virtual_address)].address;
    if (!pdp) {
        pdp = MmAllocate(sizeof(PageTable));
        MmZeroMemory(pdp, sizeof(PageTable));
        PageDirectoryEntry *pdp_entry = &kPML4->pde[MmGetPdp(virtual_address)];
        pdp_entry->address = (uint64_t) pdp;
        pdp_entry->present = 1;
        pdp_entry->read_write = 1;
    }

    PageTable *pd = (PageTable *) (uintptr_t) pdp->pde[MmGetPd(virtual_address)].address;
    if (!pd) {
        pd = MmAllocate(sizeof(PageTable));
        MmZeroMemory(pd, sizeof(PageTable));
        PageDirectoryEntry *pd_entry = &pdp->pde[MmGetPd(virtual_address)];
        pd_entry->address = (uint64_t) pd;
        pd_entry->present = 1;
        pd_entry->read_write = 1;
    }

    PageTable *pt = (PageTable *) (uintptr_t) pd->pde[MmGetPt(virtual_address)].address;
    if (!pt) {
        pt = MmAllocate(sizeof(PageTable));
        MmZeroMemory(pt, sizeof(PageTable));
        PageDirectoryEntry *pt_entry = &pd->pde[MmGetPt(virtual_address)];
        pt_entry->address = (uint64_t) pt;
        pt_entry->present = 1;
        pt_entry->read_write = 1;
    }

    PageDirectoryEntry *pi_entry = &pt->pde[MmGetPi(virtual_address)];
    pi_entry->address = physical_address;
    pi_entry->present = 1;
    pi_entry->read_write = 1;
    if (flags & kPageDisableCache)
        pi_entry->disable_cache = 1;
    if (flags & kPageWriteThrough)
        pi_entry->write_through = 1;
}