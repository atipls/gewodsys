#include "intel.h"

#include <lib/memory.h>
#include <utl/serial.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

// Offset to code64 in GDT
#define KERNEL_CS 0x28

__attribute__((aligned(0x1000))) static InterruptDescriptor kIntelIDT[256];
__attribute__((aligned(0x1000))) static GlobalDescriptorTable kIntelGDT = {
        {0, 0, 0, 0, 0, 0},                // null
        {0xFFFF, 0, 0, 0x9A, 0x80, 0},     // 16-bit code
        {0xFFFF, 0, 0, 0x92, 0x80, 0},     // 16-bit data
        {0xFFFF, 0, 0, 0x9A, 0xCF, 0},     // 32-bit code
        {0xFFFF, 0, 0, 0x92, 0xCF, 0},     // 32-bit data
        {0x0000, 0, 0, 0x9A, 0xA2, 0},     // 64-bit code
        {0x0000, 0, 0, 0x92, 0xA0, 0},     // 64-bit data
        {0x0000, 0, 0, 0xF2, 0x00, 0},     // user data
        {0x0000, 0, 0, 0xFA, 0x20, 0},     // user code
        {0x0068, 0, 0, 0x89, 0x20, 0, 0, 0}// tss
};

static InterruptTableDescriptor kIntelIDTR = {
        256 * sizeof(InterruptDescriptor) - 1,
        kIntelIDT,
};

static GlobalDescriptorTableDescriptor kIntelGDTR = {
        sizeof(kIntelGDT) - 1,
        &kIntelGDT,
};

static TaskStateSegment kIntelTSS;

extern uintptr_t kIntelIsrTable;

void IntelSetInterrupt(int interrupt, uint64_t handler, uint16_t type) {
    InterruptDescriptor *idt = &kIntelIDT[interrupt];
    RtZeroMemory(idt, sizeof(InterruptDescriptor));

    idt->offset0 = (uint16_t) (handler & 0xFFFF);
    idt->offset1 = (uint16_t) ((handler >> 16) & 0xFFFF);
    idt->offset2 = (uint32_t) ((handler >> 32) & 0xFFFFFFFF);

    idt->selector = KERNEL_CS;
    idt->type = type;

    idt->present = 1;
}

static void IntelInitializePic() {
    IoOut8(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    IoOut8(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);

    IoOut8(PIC1_DATA, 0x20);
    IoOut8(PIC2_DATA, 0x28);

    IoOut8(PIC1_DATA, 4);
    IoOut8(PIC2_DATA, 2);

    IoOut8(PIC1_DATA, ICW4_8086);
    IoOut8(PIC2_DATA, ICW4_8086);

    IoOut8(PIC1_DATA, 0xff);
    IoOut8(PIC2_DATA, 0xff);
}

void IntelInitialize(uint64_t kernel_stack) {
    ComPrint("[CPU] Initializing Intel CPU (stack 0x%X).\n", kernel_stack);

    kIntelTSS.rsp[0] = kernel_stack;
    kIntelTSS.ist[1] = 0;

    kIntelTSS.iopb_offset = sizeof(kIntelTSS);

    kIntelGDT.tss.base_low16 = (uintptr_t) &kIntelTSS & 0xFFFF;
    kIntelGDT.tss.base_mid8 = ((uintptr_t) &kIntelTSS >> 16) & 0xFF;
    kIntelGDT.tss.base_high8 = ((uintptr_t) &kIntelTSS >> 24) & 0xFF;
    kIntelGDT.tss.base_upper32 = (uintptr_t) &kIntelTSS >> 32;

    __asm__ volatile("lgdt %0" ::"m"(kIntelGDTR));

    ComPrint("[CPU] TSS and GDT initialized.\n");

    uint64_t isr_table = (uint64_t) &kIntelIsrTable;
    for (int vector = 0; vector < 256; vector++) {
        IntelSetInterrupt(vector, isr_table, GATE_INTR);
        isr_table += 32;
    }

    __asm__ volatile("lidt %0" ::"m"(kIntelIDTR));

    IntelInitializePic();

    IoOut8(PIC1_DATA, 0xFD);
    IoOut8(PIC2_DATA, 0xFF);

    __asm__ volatile("sti");

    ComPrint("[CPU] Interrupts enabled.\n");
}

void *IntelGetCR3(void) {
    void *cr3;
    __asm__ volatile("movq %%cr3, %0"
                     : "=r"(cr3));
    return cr3;
}

void IntelSetCR3(void *cr3) {
    __asm__ volatile("movq %0, %%cr3"
                     :
                     : "r"(cr3));
}