#pragma once

#include <cpu/acpi.h>
#include <stdint.h>

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

void MmInitializePaging(void);

uint64_t MmMapToVirtual(uint64_t physical_address, uint64_t size);

uint64_t MmGetPhysicalAddress(uint64_t virtual_address);

void MmUnmapVirtual(uint64_t virtual_address, uint64_t size);

void MmMapPage(uint64_t virtual_address, uint64_t physical_address, uint64_t flags);