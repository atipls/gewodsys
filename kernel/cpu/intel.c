#include <utl/serial.h>

#include "intel.h"

__attribute__((aligned(0x10))) static InterruptDescriptor kIntelIDT[256];
__attribute__((aligned(0x10))) static GlobalDescriptorTable kIntelGDT = {
        {0, 0, 0, 0, 0, 0},              // null
        {0xffff, 0, 0, 0x9a, 0x80, 0},   // 16-bit code
        {0xffff, 0, 0, 0x92, 0x80, 0},   // 16-bit data
        {0xffff, 0, 0, 0x9a, 0xcf, 0},   // 32-bit code
        {0xffff, 0, 0, 0x92, 0xcf, 0},   // 32-bit data
        {0, 0, 0, 0x9a, 0xa2, 0},        // 64-bit code
        {0, 0, 0, 0x92, 0xa0, 0},        // 64-bit data
        {0, 0, 0, 0xF2, 0, 0},           // user data
        {0, 0, 0, 0xFA, 0x20, 0},        // user code
        {0x68, 0, 0, 0x89, 0x20, 0, 0, 0}// tss
};

static InterruptTableDescriptor kIntelIDTR = {
        sizeof(InterruptDescriptor) * 256 - 1,
        &kIntelIDT[0]};

static GlobalDescriptorTableDescriptor kIntelGDTR = {
        sizeof(kIntelGDT) - 1,
        &kIntelGDT};

static TaskStateSegment kIntelTSS;


static void IntelSetInterrupt(uint8_t interrupt, void *handler, uint8_t flags) {
    kIntelIDT[interrupt].isr_low = (uint64_t) handler & 0xFFFF;
    kIntelIDT[interrupt].kernel_cs = 0x28;
    kIntelIDT[interrupt].ist = 0;
    kIntelIDT[interrupt].attributes = flags;
    kIntelIDT[interrupt].isr_mid = ((uint64_t) handler >> 16) & 0xFFFF;
    kIntelIDT[interrupt].isr_high = ((uint64_t) handler >> 32) & 0xFFFFFFFF;
    kIntelIDT[interrupt].reserved = 0;
}

static void IntelInitializeTimer(uint16_t frequency) {
    uint16_t divisor = 1193180 / frequency;
    IoOut(0x43, 0x36);
    IoOut(0x40, divisor & 0xFF);
    IoOut(0x40, divisor >> 8);
}

void IntelInitialize(uint64_t kernel_stack) {
    ComPrint("[CPU]: Initializing Intel CPU (stack 0x%X).\n", kernel_stack);

    kIntelTSS.rsp[0] = kernel_stack;
    kIntelTSS.ist[1] = 0;

    kIntelTSS.iopb_offset = sizeof(kIntelTSS);

    kIntelGDT.tss.base_low16 = (uintptr_t) &kIntelTSS & 0xFFFF;
    kIntelGDT.tss.base_mid8 = ((uintptr_t) &kIntelTSS >> 16) & 0xFF;
    kIntelGDT.tss.base_high8 = ((uintptr_t) &kIntelTSS >> 24) & 0xFF;
    kIntelGDT.tss.base_upper32 = (uintptr_t) &kIntelTSS >> 32;

    __asm__ volatile("lgdt %0"
                     :
                     : "m"(kIntelGDTR));

    ComPrint("[CPU]: TSS and GDT initialized.\n");

    extern void *kIntelIsrTable[32];
    extern void *kIntelIsrPicTable[32];

    for (uint8_t vector = 0; vector < 32; vector++)
        IntelSetInterrupt(vector, kIntelIsrTable[vector], 0x8E);

    for (uint8_t vector = 32; vector < 48; vector++)
        IntelSetInterrupt(vector, kIntelIsrPicTable[vector - 32], 0x8E);

    __asm__ volatile("lidt %0"
                     :
                     : "m"(kIntelIDTR));


    ComPrint("[CPU]: IDT initialized.\n");

    IntelRemapPic();
    IoOut(PIC1_DATA, 0x11); // ICW1
    IoOut(PIC2_DATA, 0x11);


    uint8_t val = IoIn(0x4D1);
    IoOut(0x4D1, val | (1 << (10-8)) | (1 << (11-8)));

    IntelInitializeTimer(1000); // 1ms

    __asm__ volatile("sti");

    ComPrint("[CPU]: Interrupts enabled.\n");
}


uint8_t IoIn(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0"
                     : "=a"(ret)
                     : "Nd"(port));
    return ret;
}

void IoOut(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" ::"a"(val), "Nd"(port));
}

void IoWait(void) {
    __asm__ volatile("outb %%al, $0x80"
                     :
                     : "a"(0));
}