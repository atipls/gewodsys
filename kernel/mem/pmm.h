#pragma once

#include "stdint.h"

#define PAGE_SIZE 0x1000

typedef struct __attribute__((packed)) {
    uint64_t total_available_pages;
    uint64_t first_available_page_addr;
    uint64_t last_available_page_address;
    uint64_t total_memory;
    uint64_t total_pages;
    uint8_t * bitfield;

    uint64_t free_memory;
    uint64_t locked_memory;
    uint64_t reserved_memory;
} MemoryStatistics;

int MmInitialize();

void MmReservePage(void* address);
void MmUnreservePage(void* address);

void MmLockPage(void* address);
void MmUnlockPage(void* address);

void MmReservePages(void* address, uint64_t pages);
void MmUnreservePages(void* address, uint64_t pages);

void MmLockPages(void* address, uint64_t pages);
void MmUnlockPages(void* address, uint64_t pages);

void* MmRequestPage();
void MmFreePage(void* address);