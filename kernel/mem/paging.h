#pragma once

#include <cpu/acpi.h>
#include <stdint.h>
#include <limine.h>

typedef struct {
    uint8_t present : 1;
    uint8_t read_write : 1;
    uint8_t superuser : 1;
    uint8_t write_through : 1;
    uint8_t disable_cache : 1;
    uint8_t accessed : 1;
    uint8_t ignore0 : 1;
    uint8_t larger_pages : 1;
    uint8_t ingore1 : 1;
    uint8_t available : 3;
    uint64_t address : 52;
} PageDirectoryEntry;

typedef struct __attribute__((aligned(0x1000))) {
    PageDirectoryEntry pde[512];
} PageTable;

enum {
    kPagePresent = 1 << 0,
    kPageReadWrite = 1 << 1,
    kPageWriteThrough = 1 << 3,
    kPageDisableCache = 1 << 4,
};

void MmInitializePaging(struct limine_memmap_response *memmap);

uint64_t MmMapToVirtual(uint64_t physical_address, uint64_t size);

uint64_t MmGetPhysicalAddress(uint64_t virtual_address);

void MmUnmapVirtual(uint64_t virtual_address, uint64_t size);

void MmMapPage(uint64_t virtual_address, uint64_t physical_address, uint64_t flags);