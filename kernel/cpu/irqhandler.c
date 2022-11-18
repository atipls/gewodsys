#include "apic.h"
#include "intel.h"

#include <utl/serial.h>

#define IRQ_NUM_VECTORS 256
#define IRQ_NUM_ISA 16
#define IRQ_VECTOR_BASE 32

typedef struct {
    uint8_t ignored;
    uint8_t type;
    union {
        void *ptr;
        IrqHandlerFn handler;
    };
    void *data;
} IrqHandlerEntry;

typedef struct {
    uint8_t used;
    uint8_t dest_irq;
    uint16_t flags;
} IrqIsaOverride;

static char *kIsrNames[];

static IrqHandlerEntry kIrqHandlers[IRQ_NUM_VECTORS];
static IrqIsaOverride kIrqIsaOverrides[IRQ_NUM_ISA];

static int kApicHighestIrq = 0;

static uint8_t kIrqSwBitmap[IRQ_NUM_VECTORS / 8];
static uint8_t kIrqHwBitmap[IRQ_NUM_VECTORS / 8];

#define SW_BITMAP_GET(index) (kIrqSwBitmap[(index) / 8] & (1 << ((index) % 8)))
#define SW_BITMAP_SET(index) (kIrqSwBitmap[(index) / 8] |= (1 << ((index) % 8)))

#define HW_BITMAP_GET(index) (kIrqHwBitmap[(index) / 8] & (1 << ((index) % 8)))
#define HW_BITMAP_SET(index) (kIrqHwBitmap[(index) / 8] |= (1 << ((index) % 8)))

static uint8_t kIpiVector;

extern uint64_t kLocalApicAddress;

__attribute__((used)) void IrqHandler(uint8_t vector) {
    // HACK HACK
    uint32_t volatile *eoi = (uint32_t volatile *)(kLocalApicAddress + 0xB0);
    *eoi = 0;

    if (kIrqHandlers[vector].handler)
        kIrqHandlers[vector].handler(vector - IRQ_VECTOR_BASE, kIrqHandlers[vector].data);
}

__attribute__((used)) void ExcHandler(uint8_t vector, uint32_t error, CpuStack *frame, CpuRegisters *regs) {
    // HACK HACK
    uint32_t volatile *eoi = (uint32_t volatile *)(kLocalApicAddress + 0xB0);
    *eoi = 0;

    if (kIrqHandlers[vector].handler)
        kIrqHandlers[vector].handler(vector, kIrqHandlers[vector].data);

    ComPrint("[INTR] Exception %d (%s)!\n", vector, kIsrNames[vector]);

    if (vector != 8) {
        ComPrint("[INTR]    Error code: %d\n", error);
        ComPrint("[INTR]    RAX: %X RBX: %X RCX: %X RDX: %X\n", regs->rax, regs->rbx, regs->rcx, regs->rdx);
        ComPrint("[INTR]    RSI: %X RDI: %X RBP: %X\n", regs->rsi, regs->rdi, regs->rbp);
        uint64_t cr2;
        __asm__ volatile("mov %%cr2, %0"
                         : "=r"(cr2));
        ComPrint("[INTR]    CR2: %X\n", cr2);
    }

    while (1) {
        __asm__ volatile("hlt");
    }
}

void ApicInitializeInterrupts(void) {
    kApicHighestIrq = ApicGetHighestIrq();

    ComPrint("[INTR] Highest IRQ: %d\n", kApicHighestIrq);

    for (int bm = IRQ_NUM_VECTORS - IRQ_VECTOR_BASE; bm < IRQ_NUM_VECTORS; bm++) {
        SW_BITMAP_SET(bm);
        HW_BITMAP_SET(bm);
    }

    for (int bm = 0; bm < IRQ_NUM_ISA; bm++)
        HW_BITMAP_SET(bm);

    // Mask out all but the highest IRQ
    for (int bm = kApicHighestIrq + 1; bm < IRQ_NUM_VECTORS - IRQ_VECTOR_BASE + 1; bm++)
        HW_BITMAP_SET(bm);

    for (int irq = 0; irq < IRQ_NUM_ISA; irq++) {
        if (kIrqIsaOverrides[irq].used) {
            uint8_t vector = kIrqIsaOverrides[irq].dest_irq + IRQ_VECTOR_BASE;
            uint16_t flags = kIrqIsaOverrides[irq].flags;
            ApicSetIrqIsaRouting(irq, vector, flags);
        } else {
            ApicSetIrqVector(irq, irq + IRQ_VECTOR_BASE);
        }
    }

    kIpiVector = IRQ_NUM_VECTORS - 1;
    SW_BITMAP_SET(kIpiVector - IRQ_VECTOR_BASE);

    ApicEnableInterrupt(kIpiVector);
}


int ApicEnableInterrupt(uint8_t irq) {
    if (irq > IRQ_NUM_VECTORS - IRQ_VECTOR_BASE)
        return -1;

    uint8_t vector = irq + IRQ_VECTOR_BASE;
    kIrqHandlers[vector].ignored = 0;

    if (irq <= kApicHighestIrq)
        ApicSetIrqMask(irq, 0);

    return 0;
}

int ApicDisableInterrupt(uint8_t irq) {
    if (irq > IRQ_NUM_VECTORS - IRQ_VECTOR_BASE)
        return -1;

    uint8_t vector = irq + IRQ_VECTOR_BASE;
    kIrqHandlers[vector].ignored = 1;

    if (irq <= kApicHighestIrq)
        ApicSetIrqMask(irq, 1);

    return 0;
}


int ApicEnableMsiInterrupt(uint8_t irq, uint8_t index, PciDevice *device) {
    if (irq > IRQ_NUM_VECTORS - IRQ_VECTOR_BASE)
        return -1;

    int result = ApicEnableInterrupt(irq);
    if (result < 0)
        return result;

    uint8_t vector = irq + IRQ_VECTOR_BASE;
    PciEnableMsiVector(device, index, vector);
    return 0;
}

int ApicDisableMsiInterrupt(uint8_t irq, uint8_t index, PciDevice *device) {
    if (irq > IRQ_NUM_VECTORS - IRQ_VECTOR_BASE)
        return -1;

    int result = ApicDisableInterrupt(irq);
    if (result < 0)
        return result;

    PciDisableMsiVector(device, index);
    return 0;
}

static uint8_t ApicIrqMapToVector(uint8_t irq) {
    uint8_t vector = irq + IRQ_VECTOR_BASE;
    if (irq <= kApicHighestIrq) {
        if (irq < IRQ_NUM_ISA && kIrqIsaOverrides[irq].used) {
            vector = kIrqIsaOverrides[irq].dest_irq + IRQ_VECTOR_BASE;
            uint16_t flags = kIrqIsaOverrides[irq].flags;
            ApicSetIrqIsaRouting(irq, vector, flags);
        } else {
            ApicSetIrqVector(irq, irq + IRQ_VECTOR_BASE);
        }
    }
    return vector;
}

int ApicAllocateSoftwareIrq() {
    for (int bm = 0; bm < IRQ_NUM_VECTORS; bm++) {
        if (SW_BITMAP_GET(bm)) {
            SW_BITMAP_SET(bm);
            return bm;
        }
    }

    return -1;
}

void ApicRegisterIrqHandler(uint8_t irq, IrqHandlerFn handler, void *data) {
    uint8_t vector = ApicIrqMapToVector(irq);
    kIrqHandlers[vector].ignored = 1;
    kIrqHandlers[vector].type = 1;
    kIrqHandlers[vector].handler = handler;
    kIrqHandlers[vector].data = data;
}


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
        [30] = "Security exception",
};
