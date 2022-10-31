#pragma once

#include "pmm.h"
#include <stdint.h>

#define PAGE_WRITE_BIT 0x1
#define PAGE_USER_BIT 0x2
#define PAGE_NX_BIT 0x4
#define PAGE_CACHE_DISABLE 0x8

#define PAGE_ALLOW_WRITE(x) ((page_set((x), PAGE_WRITE_BIT)))
#define PAGE_RESTRICT_WRITE(x) ((page_clear((x), PAGE_WRITE_BIT)))
#define PAGE_ALLOW_USER(x) ((page_set((x), PAGE_USER_BIT)))
#define PAGE_RESTRICT_USER(x) ((page_clear((x), PAGE_USER_BIT)))
#define PAGE_ALLOW_NX(x) ((page_set((x), PAGE_NX_BIT)))
#define PAGE_RESTRICT_NX(x) ((page_clear((x), PAGE_NX_BIT)))
#define PAGE_DISABLE_CACHE(x) ((page_set((x), PAGE_CACHE_DISABLE)))
#define PAGE_ENABLE_CACHE(x) ((page_clear((x), PAGE_CACHE_DISABLE)))

typedef struct __attribute__((packed)) {
    uint64_t present : 1;
    uint64_t writeable : 1;
    uint64_t user_access : 1;
    uint64_t write_through : 1;
    uint64_t cache_disabled : 1;
    uint64_t accessed : 1;
    uint64_t ignored_3 : 1;
    uint64_t is_mapped : 1;
    uint64_t ignored_2 : 4;
    uint64_t page_ppn : 28;
    uint64_t reserved_1 : 12;
    uint64_t ignored_1 : 11;
    uint64_t execution_disabled : 1;
} PageDirectoryEntry;

typedef struct __attribute__((__packed__)) {
    uint64_t present : 1;
    uint64_t writeable : 1;
    uint64_t user_access : 1;
    uint64_t write_through : 1;
    uint64_t cache_disabled : 1;
    uint64_t accessed : 1;
    uint64_t dirty : 1;
    uint64_t is_mapped : 1;
    uint64_t global : 1;
    uint64_t ignored_2 : 3;
    uint64_t page_ppn : 28;
    uint64_t reserved_1 : 12;
    uint64_t ignored_1 : 11;
    uint64_t execution_disabled : 1;
} PageTableEntry;

typedef struct __attribute__((aligned(PAGE_SIZE))) {
    PageDirectoryEntry entries[512];
} PageDirectory;

typedef struct __attribute__((aligned(PAGE_SIZE))) {
    PageTableEntry entries[512];
} PageTable;

typedef struct {
    uint64_t pdp;
    uint64_t pd;
    uint64_t pt;
    uint64_t p;
} PageMapIndex;

void MmInitializePaging();
void MmGetPageIndices(uint64_t virtual_address, PageMapIndex *map);

void MmMapMemory(void *virtual_address, void *physical_address);
uint64_t MmGetPhysicalAddress(void *virtual_address);
void *MmGetIdentityPage();

void MmPageSet(void *address, uint8_t field);
void MmPageClear(void *address, uint8_t field);

void MmDebugPrint(void *virtual_address, void *physical_address);