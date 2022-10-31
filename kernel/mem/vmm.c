#include "vmm.h"

#include <lib/memory.h>
#include <utl/serial.h>

PageDirectory *kPML4;


static void print_pde(PageDirectoryEntry *pde) {
    ComPrint("[MM] p: %d w: %d u: %d wt: %d c: %d a: %d i3: %d s: %d i2: %d [pp: 0x%x] r: %d i1: %d n: %d\n",
             pde->present, pde->writeable, pde->user_access, pde->write_through, pde->cache_disabled,
             pde->accessed, pde->ignored_3, pde->is_mapped, pde->ignored_2, pde->page_ppn, pde->reserved_1,
             pde->ignored_1, pde->execution_disabled);
}

static void print_pte(PageTableEntry *pte) {
    ComPrint("[MM] p: %d w: %d u: %d wt: %d c: %d a: %d d: %d s: %d g: %d i2: %d [pp: 0x%x] r: %d i1: %d n: %d\n",
             pte->present, pte->writeable, pte->user_access, pte->write_through, pte->cache_disabled,
             pte->accessed, pte->dirty, pte->is_mapped, pte->global, pte->ignored_2, pte->page_ppn, pte->reserved_1,
             pte->ignored_1, pte->execution_disabled);
}

void MmInitializePaging() {
    __asm__("movq %%cr3, %0"
            : "=r"(kPML4));
    __asm__("movq %0, %%cr3"
            :
            : "r"(kPML4));
}


void MmGetPageIndices(uint64_t address, PageMapIndex *map) {
    address >>= 12;
    map->p = address & 0x1ff;
    address >>= 9;
    map->pt = address & 0x1ff;
    address >>= 9;
    map->pd = address & 0x1ff;
    address >>= 9;
    map->pdp = address & 0x1ff;
}

void MmMapMemory(void *virtual_memory, void *physical_memory) {
    if ((uint64_t) virtual_memory & 0xfff)
        ComPrint("Crashing: virtual_memory: 0x%X\n", virtual_memory);

    if ((uint64_t) physical_memory & 0xfff)
        ComPrint("Crashing: physical_memory: 0x%X\n", physical_memory);

    PageMapIndex map;
    MmGetPageIndices((uint64_t) virtual_memory, &map);

    PageDirectoryEntry pde = kPML4->entries[map.pdp];

    PageDirectory *pdp;
    if (!pde.present) {
        pdp = (PageDirectory *) MmRequestPage();
        RtZeroMemory(pdp, PAGE_SIZE);

        pde.page_ppn = (uint64_t) pdp >> 12;
        pde.present = 1;
        pde.writeable = 1;
        kPML4->entries[map.pdp] = pde;
    } else {
        pdp = (PageDirectory *) ((uint64_t) pde.page_ppn << 12);
    }

    pde = pdp->entries[map.pd];
    PageDirectory *pd;
    if (!pde.present) {
        pd = (PageDirectory *) MmRequestPage();
        RtZeroMemory(pd, PAGE_SIZE);
        pde.page_ppn = (uint64_t) pd >> 12;
        pde.present = 1;
        pde.writeable = 1;
        pdp->entries[map.pd] = pde;
    } else {
        pd = (PageDirectory *) ((uint64_t) pde.page_ppn << 12);
    }

    pde = pd->entries[map.pt];
    PageTable *pt;
    if (!pde.present) {
        pt = (PageTable *) MmRequestPage();
        RtZeroMemory(pt, PAGE_SIZE);
        pde.page_ppn = (uint64_t) pt >> 12;
        pde.present = 1;
        pde.writeable = 1;
        pd->entries[map.pt] = pde;
    } else {
        pt = (PageTable *) ((uint64_t) pde.page_ppn << 12);
    }

    PageTableEntry pte = pt->entries[map.p];
    pte.page_ppn = (uint64_t) physical_memory >> 12;
    pte.present = 1;
    pte.writeable = 1;
    pt->entries[map.p] = pte;
}

uint64_t MmGetPhysicalAddress(void *virtual_memory) {
    PageMapIndex map;
    MmGetPageIndices((uint64_t) virtual_memory, &map);

    PageDirectoryEntry pde = kPML4->entries[map.pdp];
    PageDirectory *pd = (PageDirectory *) ((uint64_t) pde.page_ppn << 12);
    pde = pd->entries[map.pd];
    pd = (PageDirectory *) ((uint64_t) pde.page_ppn << 12);
    pde = pd->entries[map.pt];

    PageTable *pt = (PageTable *) ((uint64_t) pde.page_ppn << 12);
    PageTableEntry pte = pt->entries[map.p];

    return ((uint64_t) pte.page_ppn << 12) | ((uint64_t) virtual_memory & 0xfff);
}

void *MmGetIdentityPage() {
    void *result = MmRequestPage();
    MmMapMemory(result, result);
    return result;
}

void MmSetPagePermissions(void *address, uint8_t permissions) {
    PageMapIndex map;
    MmGetPageIndices((uint64_t) address, &map);

    PageDirectoryEntry pde;
    PageDirectory *pd;

    pde = kPML4->entries[map.pdp];
    pd = (PageDirectory *) ((uint64_t) pde.page_ppn << 12);
    pde = pd->entries[map.pd];
    pd = (PageDirectory *) ((uint64_t) pde.page_ppn << 12);
    pde = pd->entries[map.pt];

    PageTable *pt = (PageTable *) ((uint64_t) pde.page_ppn << 12);
    PageTableEntry *pte = &pt->entries[map.p];

    pte->writeable = (permissions & 1);
    pte->user_access = ((permissions & 2) >> 1);
    pte->execution_disabled = ((permissions & 4) >> 2);
    pte->cache_disabled = ((permissions & 8) >> 3);
}

uint8_t MmGetPagePermissions(void *address) {
    PageMapIndex map;
    MmGetPageIndices((uint64_t) address, &map);

    PageDirectoryEntry pde = kPML4->entries[map.pdp];
    PageDirectory *pd = (PageDirectory *) ((uint64_t) pde.page_ppn << 12);
    pde = pd->entries[map.pd];
    pd = (PageDirectory *) ((uint64_t) pde.page_ppn << 12);
    pde = pd->entries[map.pt];

    PageTable *pt = (PageTable *) ((uint64_t) pde.page_ppn << 12);
    PageTableEntry pte = pt->entries[map.p];

    uint8_t result = pte.writeable;
    result |= (pte.user_access << 1);
    result |= (pte.execution_disabled << 2);
    result |= (pte.cache_disabled << 3);

    return result;
}

void MmPageSet(void *address, uint8_t field) {
    uint8_t new_permissions = MmGetPagePermissions(address) | field;
    MmSetPagePermissions(address, new_permissions);
}

void MmPageClear(void *address, uint8_t field) {
    uint8_t new_permissions = MmGetPagePermissions(address) & ~field;
    MmSetPagePermissions(address, new_permissions);
}

void MmDebugPrint(void *virtual_memory, void *physical_memory) {
    PageMapIndex map;
    MmGetPageIndices((uint64_t) virtual_memory, &map);

    PageDirectoryEntry pde;
    PageDirectory *pd;
    ComPrint("[MAP DEBUG] Virtual memory: 0x%llx Physical target: 0x%llx\n", (uint64_t) virtual_memory, (uint64_t) physical_memory);
    ComPrint("MAP: pdp: %x pd: %x pt: %x p: %x\n", map.pdp, map.pd, map.pt, map.p);
    pde = kPML4->entries[map.pdp];
    ComPrint("PDP entry at: %p\n", &kPML4->entries[map.pdp]);
    print_pde(&pde);
    pd = (PageDirectory *) ((uint64_t) pde.page_ppn << 12);
    pde = pd->entries[map.pd];
    ComPrint("PD entry at: %p\n", &pd->entries[map.pd]);
    print_pde(&pde);
    pd = (PageDirectory *) ((uint64_t) pde.page_ppn << 12);

    pde = pd->entries[map.pt];
    ComPrint("PT entry at: %p\n", &pd->entries[map.pt]);
    print_pde(&pde);

    PageTable *pt = (PageTable *) ((uint64_t) pde.page_ppn << 12);
    PageTableEntry pte = pt->entries[map.p];
    ComPrint("P entry at: %p\n", &pt->entries[map.p]);
    print_pte(&pte);

    uint64_t physical = ((uint64_t) pte.page_ppn << 12) | ((uint64_t) virtual_memory & 0xfff);
    ComPrint("Points to physical: %llx\n", physical);
    ComPrint("I said it was:      %llx\n", physical_memory);
}
