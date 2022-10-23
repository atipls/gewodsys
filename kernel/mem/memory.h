#pragma once

#include <limine.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define KERNEL_MEMORY_SIZE 256 * 1024 * 1024

void MmZeroMemory(void *address, uint64_t size);
void MmCopyMemory(void *destination, void *source, uint64_t size);

void MmInitialize(struct limine_memmap_response *memmap);

void MmLockPage(uint64_t address);
void MmUnlockPage(uint64_t address);

void MmLockPages(uint64_t address, uint64_t size);
void MmUnlockPages(uint64_t address, uint64_t size);

void MmReservePage(uint64_t address);
void MmUnreservePage(uint64_t address);

void MmReservePages(uint64_t address, uint64_t size);
void MmUnreservePages(uint64_t address, uint64_t size);

void *MmAllocate(uint64_t size);
void MmFree(void *ptr, uint64_t size);

uint64_t MmGetTotalMemory();
uint64_t MmGetFreeMemory();
uint64_t MmGetUsedMemory();
uint64_t MmGetReservedMemory();
