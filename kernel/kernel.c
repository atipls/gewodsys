#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "limine.h"
#include <cpu/acpi.h>
#include <cpu/intel.h>

#include <mem/pmm.h>
#include <mem/vmm.h>
#include <mem/heap.h>

#include <utl/serial.h>

static volatile struct limine_framebuffer_request framebuffer_request = {
        .id = LIMINE_FRAMEBUFFER_REQUEST,
        .revision = 1,
};

static volatile struct limine_smp_request smp_request = {
        .id = LIMINE_SMP_REQUEST,
        .revision = 0,
};

#if 0
// Entry-point for secondary cores
static void KeSMPMain(struct limine_smp_info *info) {
    ComPrint("Secondary core %d started\n", info->processor_id);

    while (1) {
        // Wait for an interrupt or a task to be scheduled
        __asm__ volatile("hlt");
    }
}
#endif

// Entry-point for primary core
void KeMain(void) {
    uint64_t stack = 0;
    __asm__ volatile("mov %%rsp, %0"
                     : "=r"(stack));

    ComPrint("[KERNEL] Primary core started.\n", stack);

    MmInitialize();
    MmInitializePaging();
    MmInitializeHeap();

    IntelInitialize(stack);
    AcpiInitialize();

    // TskInitialize();

#if 0
    // Start up the other cores
    struct limine_smp_response *smp = smp_request.response;
    for (size_t i = 0; i < smp->cpu_count; i++)
        smp->cpus[i]->goto_address = KeSMPMain;
#endif

    // Main loop
    while (1) __asm__ volatile("hlt");
}
