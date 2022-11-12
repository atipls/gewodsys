#include "apic.h"

uint64_t kApicAddress = 0;

static uint32_t ApicRead(uint32_t reg) {
    return *(volatile uint32_t *) (kApicAddress + reg);
}

static void ApicWrite(uint32_t reg, uint32_t value) {
    *(volatile uint32_t *) (kApicAddress + reg) = value;
}

void ApicInitialize(uint64_t address) {
    kApicAddress = address;

    ApicWrite(0x0080, 0); // Clear out the task priority register

    ApicWrite(0x00e0, 0xFFFFFFFF); // Flat delivery mode
    ApicWrite(0x00d0, 0x01000000);

    ApicWrite(0x00f0, 0x100 | 0xFF); // Enable spurious interrupt vector 0x100

    // Set the IRQ
    ApicWrite(0x0320, 0x20); // Timer
}

