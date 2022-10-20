#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "limine.h"
#include <cpu/intel.h>
#include <cpu/acpi.h>

#include <mem/memory.h>
#include <utl/serial.h>

static volatile struct limine_framebuffer_request framebuffer_request = {
        .id = LIMINE_FRAMEBUFFER_REQUEST,
        .revision = 1,
};

static volatile struct limine_smp_request smp_request = {
        .id = LIMINE_SMP_REQUEST,
        .revision = 0,
};

static volatile struct limine_memmap_request memmap_request = {
        .id = LIMINE_MEMMAP_REQUEST,
        .revision = 0,
};


// Entry-point for secondary cores
static void KeSMPMain(struct limine_smp_info *info) {
    ComPrint("Secondary core %d started\n", info->processor_id);


    uint32_t i;

    while (1) {
        struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
        uint32_t *pixels = (uint32_t *) framebuffer->address;
        for (size_t y = 0; y < framebuffer->height; y++) {
            for (size_t x = 0; x < framebuffer->width; x++) {
                pixels[y * framebuffer->width + x] = x ^ y ^ i++ ^ info->processor_id;
            }
        }
    }
}

// Entry-point for primary core
void KeMain(void) {
    uint64_t stack = 0; __asm__ volatile("mov %%rsp, %0" : "=r"(stack));

    ComPrint("[KERNEL] Primary core started.\n", stack);

    IntelInitialize(stack);
    MmInitialize(memmap_request.response);
    AcpiInitialize();
    // TskInitialize();

#if 1
    // Start up the other cores
    struct limine_smp_response *smp = smp_request.response;
    for (size_t i = 0; i < smp->cpu_count; i++)
        smp->cpus[i]->goto_address = KeSMPMain;
#endif

    ComPrint("[KERNEL] Total memory: %d KiB.\n", MmGetTotalMemory() / 1024);
    ComPrint("[KERNEL] Free memory: %d KiB.\n", MmGetFreeMemory() / 1024);
    ComPrint("[KERNEL] Used memory: %d KiB.\n", MmGetUsedMemory() / 1024);
    ComPrint("[KERNEL] Reserved memory: %d KiB.\n", MmGetReservedMemory() / 1024);

    for (int i = 0; i < 10; i++) {
        void *ptr = MmAllocate(1024 * 1024);
        ComPrint("[KERNEL] Allocated 1MiB at 0x%X.\n", ptr);
        *(uint64_t *) ptr = 0xCAFEBABE;
    }

    uint32_t i = 0;
    while (1) {
        struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
        uint32_t *pixels = (uint32_t *) framebuffer->address;
        for (size_t y = 0; y < framebuffer->height; y++) {
            for (size_t x = 0; x < framebuffer->width; x++) {
                pixels[y * framebuffer->width + x] = x ^ y ^ i++;
            }
        }
    }
}
