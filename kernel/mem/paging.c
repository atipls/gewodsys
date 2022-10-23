#include "paging.h"
#include "memory.h"

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

void MmInitializePaging(void) {
    kPML4 = MmAllocate(sizeof(PageTable));
    MmZeroMemory(kPML4, sizeof(PageTable));

    struct limine_kernel_address_response *kernel_address = kernel_address_request.response;

    ComPrint("Kernel text: 0x%X - 0x%X\n", &kTextStart, &kTextEnd);
    ComPrint("Kernel rodata: 0x%X - 0x%X\n", &kRodataStart, &kRodataEnd);
    ComPrint("Kernel data: 0x%X - 0x%X\n", &kDataStart, &kDataEnd);
    ComPrint("Kernel size: 0x%X\n", &kKernelSize);

    while (1) __asm__("hlt");


    __asm__ volatile("mov %0, %%cr3" ::"r"(kPML4)
                     : "memory");

    ComPrint("[MM]: Paging initialized.\n");
}

uint64_t MmMapToVirtual(uint64_t physical_address, uint64_t size) {
    uint64_t virtual_address = (uint64_t) MmAllocate(size);
    uint64_t virtual_address_start = virtual_address;
    while (virtual_address < virtual_address_start + size) {
        MmMapPage(virtual_address, physical_address, 0);
        virtual_address += 0x1000;
        physical_address += 0x1000;
    }
    return virtual_address_start;
}

uint64_t MmGetPhysicalAddress(uint64_t virtual_address) {
    PageDirectoryEntry *pdp_entry = &kPML4->pde[MmGetPdp(virtual_address)];
    if (!pdp_entry->present)
        return 0;

    PageTable *pd_table = (PageTable *) (pdp_entry->address << 12);
    PageDirectoryEntry *pd_entry = &pd_table->pde[MmGetPd(virtual_address)];
    if (!pd_entry->present)
        return 0;

    PageTable *pt_table = (PageTable *) (pd_entry->address << 12);
    PageDirectoryEntry *pt_entry = &pt_table->pde[MmGetPt(virtual_address)];
    if (!pt_entry->present)
        return 0;

    PageTable *pi_table = (PageTable *) (pt_entry->address << 12);
    PageDirectoryEntry *pi_entry = &pi_table->pde[MmGetPi(virtual_address)];
    if (!pi_entry->present)
        return 0;

    return pi_entry->address << 12;
}

void MmUnmapVirtual(uint64_t virtual_address, uint64_t size) {
    uint64_t virtual_address_start = virtual_address;
    while (virtual_address < virtual_address_start + size) {
        PageDirectoryEntry *pdp_entry = &kPML4->pde[MmGetPdp(virtual_address)];
        if (!pdp_entry->present)
            return;

        PageTable *pd_table = (PageTable *) (pdp_entry->address << 12);
        PageDirectoryEntry *pd_entry = &pd_table->pde[MmGetPd(virtual_address)];
        if (!pd_entry->present)
            return;

        PageTable *pt_table = (PageTable *) (pd_entry->address << 12);
        PageDirectoryEntry *pt_entry = &pt_table->pde[MmGetPt(virtual_address)];
        if (!pt_entry->present)
            return;

        PageTable *pi_table = (PageTable *) (pt_entry->address << 12);
        PageDirectoryEntry *pi_entry = &pi_table->pde[MmGetPi(virtual_address)];
        if (!pi_entry->present)
            return;

        pi_entry->present = 0;
        virtual_address += 0x1000;
    }
}

void MmMapPage(uint64_t virtual_address, uint64_t physical_address, uint64_t flags) {
    (void) flags;
    PageDirectoryEntry *pdp = &kPML4->pde[MmGetPdp(virtual_address)];
    if (!pdp->present) {
        pdp->present = 1;
        pdp->read_write = 1;
        pdp->address = (uint64_t) MmAllocate(sizeof(PageTable)) >> 12;
    }

    PageTable *pd = (PageTable *) (pdp->address << 12);
    PageDirectoryEntry *pde = &pd->pde[MmGetPd(virtual_address)];

    if (!pde->present) {
        pde->present = 1;
        pde->read_write = 1;
        pde->address = (uint64_t) MmAllocate(sizeof(PageTable)) >> 12;
    }

    PageTable *pt = (PageTable *) (pde->address << 12);
    PageDirectoryEntry *pte = &pt->pde[MmGetPt(virtual_address)];

    if (!pte->present) {
        pte->present = 1;
        pte->read_write = 1;
        pte->address = (uint64_t) MmAllocate(sizeof(PageTable)) >> 12;
    }

    PageTable *pi = (PageTable *) (pte->address << 12);
    PageDirectoryEntry *pie = &pi->pde[MmGetPi(virtual_address)];
    if (!pie->present) {
        pie->present = 1;
        pie->read_write = 1;
        pie->address = physical_address >> 12;
    }
}