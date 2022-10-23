#include <stdint.h>
#include <utl/serial.h>

#include "intel.h"

typedef struct CpuContext {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rflags;
    uint64_t rip;
    uint64_t rsp;
} CpuContext;

static char *kIsrNames[] = {
        [0] = "Division-by-zero",
        [1] = "Debug",
        [2] = "Non-maskable interrupt",
        [3] = "Breakpoint",
        [4] = "Overflow",
        [5] = "Bound range exceeded",
        [6] = "Invalid opcode",
        [7] = "Device not available",
        [8] = "Double fault",
        [9] = "Coprocessor segment overrun",
        [10] = "Invalid TSS",
        [11] = "Segment not present",
        [12] = "Stack fault",
        [13] = "General protection fault",
        [14] = "Page fault",
        [16] = "x87 floating-point exception",
        [17] = "Alignment check",
        [18] = "Machine check",
        [19] = "SIMD floating-point exception",
        [20] = "Virtualization exception",
        [30] = "Security exception"};


__attribute__((no_caller_saved_registers)) static void IsrGenericHandler(CpuContext *ctx, uint32_t isr_number) {
    ComPrint("Unhandled interrupt: %s\n", kIsrNames[isr_number]);

    if (isr_number == 14) {
        uint64_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        ComPrint("Page fault at address: %X\n", cr2);

        uint64_t error_code = ctx->rax;
        ComPrint("Error code: %X\n", error_code);
    }

    ComPrint("RAX: 0x%X  RBX: 0x%X  RCX: 0x%X  RDX: 0x%X\n", ctx->rax, ctx->rbx, ctx->rcx, ctx->rdx);
    ComPrint("RSI: 0x%X  RDI: 0x%X  RBP: 0x%X  R8: 0x%X\n", ctx->rsi, ctx->rdi, ctx->rbp, ctx->r8);
    ComPrint("R9: 0x%X  R10: 0x%X  R11: 0x%X  R12: 0x%X\n", ctx->r9, ctx->r10, ctx->r11, ctx->r12);
    ComPrint("R13: 0x%X  R14: 0x%X  R15: 0x%X  RFLAGS: 0x%X\n", ctx->r13, ctx->r14, ctx->r15, ctx->rflags);
    ComPrint("RIP: 0x%X  RSP: 0x%X\n", ctx->rip, ctx->rsp);

    while (1)
        __asm__ volatile("hlt");
}

__attribute__((interrupt)) static void IsrHandler0(CpuContext *ctx) { IsrGenericHandler(ctx, 0); }
__attribute__((interrupt)) static void IsrHandler1(CpuContext *ctx) { IsrGenericHandler(ctx, 1); }
__attribute__((interrupt)) static void IsrHandler2(CpuContext *ctx) { IsrGenericHandler(ctx, 2); }
__attribute__((interrupt)) static void IsrHandler3(CpuContext *ctx) { IsrGenericHandler(ctx, 3); }
__attribute__((interrupt)) static void IsrHandler4(CpuContext *ctx) { IsrGenericHandler(ctx, 4); }
__attribute__((interrupt)) static void IsrHandler5(CpuContext *ctx) { IsrGenericHandler(ctx, 5); }
__attribute__((interrupt)) static void IsrHandler6(CpuContext *ctx) { IsrGenericHandler(ctx, 6); }
__attribute__((interrupt)) static void IsrHandler7(CpuContext *ctx) { IsrGenericHandler(ctx, 7); }
__attribute__((interrupt)) static void IsrHandler8(CpuContext *ctx) { IsrGenericHandler(ctx, 8); }
__attribute__((interrupt)) static void IsrHandler9(CpuContext *ctx) { IsrGenericHandler(ctx, 9); }
__attribute__((interrupt)) static void IsrHandler10(CpuContext *ctx) { IsrGenericHandler(ctx, 10); }
__attribute__((interrupt)) static void IsrHandler11(CpuContext *ctx) { IsrGenericHandler(ctx, 11); }
__attribute__((interrupt)) static void IsrHandler12(CpuContext *ctx) { IsrGenericHandler(ctx, 12); }
__attribute__((interrupt)) static void IsrHandler13(CpuContext *ctx) { IsrGenericHandler(ctx, 13); }
__attribute__((interrupt)) static void IsrHandler14(CpuContext *ctx) { IsrGenericHandler(ctx, 14); }
__attribute__((interrupt)) static void IsrHandler15(CpuContext *ctx) { IsrGenericHandler(ctx, 15); }
__attribute__((interrupt)) static void IsrHandler16(CpuContext *ctx) { IsrGenericHandler(ctx, 16); }
__attribute__((interrupt)) static void IsrHandler17(CpuContext *ctx) { IsrGenericHandler(ctx, 17); }
__attribute__((interrupt)) static void IsrHandler18(CpuContext *ctx) { IsrGenericHandler(ctx, 18); }
__attribute__((interrupt)) static void IsrHandler19(CpuContext *ctx) { IsrGenericHandler(ctx, 19); }
__attribute__((interrupt)) static void IsrHandler20(CpuContext *ctx) { IsrGenericHandler(ctx, 20); }
__attribute__((interrupt)) static void IsrHandler21(CpuContext *ctx) { IsrGenericHandler(ctx, 21); }
__attribute__((interrupt)) static void IsrHandler22(CpuContext *ctx) { IsrGenericHandler(ctx, 22); }
__attribute__((interrupt)) static void IsrHandler23(CpuContext *ctx) { IsrGenericHandler(ctx, 23); }
__attribute__((interrupt)) static void IsrHandler24(CpuContext *ctx) { IsrGenericHandler(ctx, 24); }
__attribute__((interrupt)) static void IsrHandler25(CpuContext *ctx) { IsrGenericHandler(ctx, 25); }
__attribute__((interrupt)) static void IsrHandler26(CpuContext *ctx) { IsrGenericHandler(ctx, 26); }
__attribute__((interrupt)) static void IsrHandler27(CpuContext *ctx) { IsrGenericHandler(ctx, 27); }
__attribute__((interrupt)) static void IsrHandler28(CpuContext *ctx) { IsrGenericHandler(ctx, 28); }
__attribute__((interrupt)) static void IsrHandler29(CpuContext *ctx) { IsrGenericHandler(ctx, 29); }
__attribute__((interrupt)) static void IsrHandler30(CpuContext *ctx) { IsrGenericHandler(ctx, 30); }
__attribute__((interrupt)) static void IsrHandler31(CpuContext *ctx) { IsrGenericHandler(ctx, 31); }

__attribute__((no_caller_saved_registers)) static void IsrPicEoi(unsigned char irq) {
    ComPrint("IsrPicEoi: irq=%d \n", irq);

    if (irq >= 8)
        IoOut8(PIC2_COMMAND, PIC_EOI);

    IoOut8(PIC1_COMMAND, PIC_EOI);
}

__attribute__((interrupt)) static void IsrPicHandler0(CpuContext *ctx) {
    (void) ctx;

    ComPrint("IsrPicHandler0 - Timer!\n");

    IsrPicEoi(0);
}

__attribute__((interrupt)) static void IsrPicHandler1(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(1);

    uint8_t state = IoIn8(0x60);
    uint8_t scancode = state & 0x7F;
    uint8_t pressed = !(state & 0x80);

    ComPrint("IsrPicHandler1 - Keyboard! scancode=%d pressed=%d\n", scancode, pressed);
}

__attribute__((interrupt)) static void IsrPicHandler2(CpuContext *ctx) {
    (void) ctx;

    IsrPicEoi(1);
}

__attribute__((interrupt)) static void IsrPicHandler3(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(3);
}

__attribute__((interrupt)) static void IsrPicHandler4(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(4);

}

__attribute__((interrupt)) static void IsrPicHandler5(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(5);
}

__attribute__((interrupt)) static void IsrPicHandler6(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(6);
}

__attribute__((interrupt)) static void IsrPicHandler7(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(7);
}

__attribute__((interrupt)) static void IsrPicHandler8(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(8);
}

__attribute__((interrupt)) static void IsrPicHandler9(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(9);
}

__attribute__((interrupt)) static void IsrPicHandler10(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(10);
}

__attribute__((interrupt)) static void IsrPicHandler11(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(11);
}

__attribute__((interrupt)) static void IsrPicHandler12(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(12);
}

__attribute__((interrupt)) static void IsrPicHandler13(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(13);
}

__attribute__((interrupt)) static void IsrPicHandler14(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(14);
}

__attribute__((interrupt)) static void IsrPicHandler15(CpuContext *ctx) {
    (void) ctx;
    IsrPicEoi(15);
}

void IntelRemapPic(void) {
    uint8_t a1, a2;

    a1 = IoIn8(PIC1_DATA);
    IoWait();
    a2 = IoIn8(PIC2_DATA);
    IoWait();

    IoOut8(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    IoWait();
    IoOut8(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    IoWait();

    IoOut8(PIC1_DATA, 0x20);
    IoWait();
    IoOut8(PIC2_DATA, 0x28);
    IoWait();

    IoOut8(PIC1_DATA, 4);
    IoWait();
    IoOut8(PIC2_DATA, 2);
    IoWait();

    IoOut8(PIC1_DATA, ICW4_8086);
    IoWait();
    IoOut8(PIC2_DATA, ICW4_8086);
    IoWait();

    IoOut8(PIC1_DATA, a1);
    IoWait();
    IoOut8(PIC2_DATA, a2);
}

void *kIntelIsrTable[32] = {
        IsrHandler0,
        IsrHandler1,
        IsrHandler2,
        IsrHandler3,
        IsrHandler4,
        IsrHandler5,
        IsrHandler6,
        IsrHandler7,
        IsrHandler8,
        IsrHandler9,
        IsrHandler10,
        IsrHandler11,
        IsrHandler12,
        IsrHandler13,
        IsrHandler14,
        IsrHandler15,
        IsrHandler16,
        IsrHandler17,
        IsrHandler18,
        IsrHandler19,
        IsrHandler20,
        IsrHandler21,
        IsrHandler22,
        IsrHandler23,
        IsrHandler24,
        IsrHandler25,
        IsrHandler26,
        IsrHandler27,
        IsrHandler28,
        IsrHandler29,
        IsrHandler30,
        IsrHandler31,
};

void *kIntelIsrPicTable[16] = {
        IsrPicHandler0,
        IsrPicHandler1,
        IsrPicHandler2,
        IsrPicHandler3,
        IsrPicHandler4,
        IsrPicHandler5,
        IsrPicHandler6,
        IsrPicHandler7,
        IsrPicHandler8,
        IsrPicHandler9,
        IsrPicHandler10,
        IsrPicHandler11,
        IsrPicHandler12,
        IsrPicHandler13,
        IsrPicHandler14,
        IsrPicHandler15,
};