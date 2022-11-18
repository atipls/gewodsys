#include "apic.h"
#include "intel.h"

#include <lib/list.h>
#include <mem/vmm.h>
#include <utl/serial.h>

#define ACPI_MADT_TYPE_LOCAL_APIC 0
#define ACPI_MADT_TYPE_IO_APIC 1
#define ACPI_MADT_TYPE_INT_SRC 2
#define ACPI_MADT_TYPE_NMI_INT_SRC 3
#define ACPI_MADT_TYPE_LAPIC_NMI 4
#define ACPI_MADT_TYPE_APIC_OVERRIDE 5

#define IOREGSEL 0x00
#define IOREGWIN 0x10

#define IOAPIC_ID 0x00
#define IOAPIC_VERSION 0x01
#define IOAPIC_ARB_ID 0x02
#define IOAPIC_RENTRY_BASE 0x10

#define IOAPIC_DEST_PHYSICAL 0
#define IOAPIC_DEST_LOGICAL 1
#define IOAPIC_IDLE 1
#define IOAPIC_PENDING 2
#define IOAPIC_ACTIVE_HIGH 0
#define IOAPIC_ACTIVE_LOW 1
#define IOAPIC_EDGE 0
#define IOAPIC_LEVEL 1

#define ISA_NUM_IRQS 16


typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t length;
} AcpiMadtEntry;

typedef struct __attribute__((packed)) {
    AcpiMadtEntry header;
    uint8_t acpi_processor_uid;
    uint8_t apic_id;
    uint32_t flags;
} AcpiMadtLocalApic;

typedef struct __attribute__((packed)) {
    AcpiMadtEntry header;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t address;
    uint32_t gsi_base;
} AcpiMadtIoApic;

typedef struct __attribute__((packed)) {
    AcpiMadtEntry header;
    uint8_t bus;
    uint8_t source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} AcpiMadtInterruptOverride;

typedef union __attribute__((packed)) {
    uint64_t raw;
    struct {
        uint64_t vector : 8;
        uint64_t deliv_mode : 3;
        uint64_t dest_mode : 1;
        uint64_t deliv_status : 1;
        uint64_t polarity : 1;
        uint64_t remote_irr : 1;
        uint64_t trigger_mode : 1;
        uint64_t mask : 1;
        uint64_t : 39;
        uint64_t dest : 8;
    };
} IoApicRentry;

typedef struct LocalApic {
    uint8_t id;
    uintptr_t phys_addr;
    uintptr_t address;

    LIST_ENTRY(struct LocalApic) list;
} LocalApic;

typedef struct IoApic {
    uint8_t id;
    uint8_t max_rentry;
    uint8_t version;
    uint32_t gsi_base;
    uintptr_t phys_addr;
    uintptr_t address;

    LIST_ENTRY(struct IoApic) list;
} IoApic;

LIST_HEAD(LocalApic) kLocalApics = LIST_HEAD_INIT;
LIST_HEAD(IoApic) kIoApics = LIST_HEAD_INIT;

uint32_t kNumLocalApics = 0, kNumIoApics = 0;

uint64_t kLocalApicAddress = 0;

static uint32_t IoApicRead(uintptr_t address, uint8_t index) {
    *(volatile uint32_t *) (address + IOREGSEL) = index;
    return *(volatile uint32_t *) (address + IOREGWIN);
}

static void IoApicWrite(uintptr_t address, uint8_t index, uint32_t value) {
    *(volatile uint32_t *) (address + IOREGSEL) = index;
    *(volatile uint32_t *) (address + IOREGWIN) = value;
}

static uint64_t IoApicRead64(uintptr_t address, uint8_t index) {
    *(volatile uint32_t *) (address + IOREGSEL) = index;
    uint64_t low = *(volatile uint32_t *) (address + IOREGWIN);
    *(volatile uint32_t *) (address + IOREGSEL) = index + 1;
    uint64_t high = *(volatile uint32_t *) (address + IOREGWIN);
    return (high << 32) | low;
}

static void IoApicWrite64(uintptr_t address, uint8_t index, uint64_t value) {
    *(volatile uint32_t *) (address + IOREGSEL) = index;
    *(volatile uint32_t *) (address + IOREGWIN) = value & 0xFFFFFFFF;
    *(volatile uint32_t *) (address + IOREGSEL) = index + 1;
    *(volatile uint32_t *) (address + IOREGWIN) = value >> 32;
}

static IoApic *IoApicGetById(uint8_t id) {
    IoApic *ioapic = 0;
    LIST_FOREACH(ioapic, &kIoApics, list) {
        if (ioapic->id == id) {
            return ioapic;
        }
    }
    return 0;
}

static IoApic *IoApicGetByIrq(uint8_t irq) {
    IoApic *ioapic = 0;
    LIST_FOREACH(ioapic, &kIoApics, list) {
        if (ioapic->gsi_base <= irq &&
            irq < ioapic->gsi_base + ioapic->max_rentry)
            return ioapic;
    }
    return 0;
}

void ApicInitialize(AcpiMadt *madt) {
    MmMapMemory((void *) (uint64_t) madt->local_apic_address, (void *) (uint64_t) madt->local_apic_address);
    kLocalApicAddress = (uint64_t) madt->local_apic_address;

    AcpiMadtInterruptOverride *overrides[ISA_NUM_IRQS] = {0};

    if (madt->flags & 1) {
        ComPrint("[ACPI] MADT PCAT Compatibility mode.\n");

        // Disable legacy PIC
        IoOut8(0x21, 0xFF);
        IoOut8(0xA1, 0xFF);

        for (int irq = 0; irq < ISA_NUM_IRQS; irq++)
            overrides[irq] = 0;
    }

    AcpiMadtEntry *entries = (AcpiMadtEntry *) (madt + 1);
    for (AcpiMadtEntry *entry = entries;
         (uint8_t *) entry < (uint8_t *) madt + madt->header.length;
         entry = (AcpiMadtEntry *) ((uint8_t *) entry + entry->length)) {

        switch (entry->type) {
            case ACPI_MADT_TYPE_LOCAL_APIC: {
                AcpiMadtLocalApic *local_apic_data = (AcpiMadtLocalApic *) entry;
                ComPrint("[APIC] LAPIC %d => %x\n", local_apic_data->apic_id, local_apic_data->flags);


                break;
            }
            case ACPI_MADT_TYPE_IO_APIC: {
                AcpiMadtIoApic *ioapic_data = (AcpiMadtIoApic *) entry;

                IoApic *ioapic = (IoApic *) kmalloc(sizeof(IoApic));
                ioapic->id = ioapic_data->io_apic_id;
                ioapic->gsi_base = ioapic_data->gsi_base;
                ioapic->phys_addr = ioapic_data->address;
                ioapic->address = ioapic->phys_addr;

                MmMapMemory((void *) ioapic->address, (void *) ioapic->address);

                uint32_t version = IoApicRead(ioapic->address, IOAPIC_VERSION);
                ioapic->max_rentry = (version >> 16) & 0xFF;
                ioapic->version = version & 0xFF;

                // Mask all interrupts.
                for (int i = 0; i <= ioapic->max_rentry; i++) {
                    IoApicRentry rentry = {.raw = IoApicRead64(ioapic->address, IOAPIC_RENTRY_BASE + 2 * i)};
                    rentry.mask = 1;
                    IoApicWrite64(ioapic->address, IOAPIC_RENTRY_BASE + 2 * i, rentry.raw);
                }

                LIST_ADD(&kIoApics, ioapic, list);
                kNumIoApics++;

                ComPrint("[APIC] IOAPIC %d at 0x%X (GSI: 0x%X)\n", ioapic_data->io_apic_id, ioapic_data->address, ioapic_data->gsi_base);
                ComPrint("[APIC]     IOAPIC Version: %d\n", ioapic->version);
                ComPrint("[APIC]     IOAPIC Max Rentry: %d\n", ioapic->max_rentry);
                break;
            }
            case ACPI_MADT_TYPE_INT_SRC: break;
            case ACPI_MADT_TYPE_NMI_INT_SRC: break;
            case ACPI_MADT_TYPE_LAPIC_NMI: break;
            case ACPI_MADT_TYPE_APIC_OVERRIDE: break;
        }
    }

    ApicInitializeInterrupts();
}

int ApicGetHighestIrq() {
    int highest = 0;
    IoApic *ioapic = 0;
    LIST_FOREACH(ioapic, &kIoApics, list) {
        if (ioapic->gsi_base + ioapic->max_rentry > highest)
            highest = ioapic->gsi_base + ioapic->max_rentry;
    }
    return highest;
}


int ApicSetIrqIsaRouting(uint8_t isa_irq, uint8_t vector, uint16_t flags) {
    IoApic *ioapic = IoApicGetByIrq(isa_irq);
    if (!ioapic)
        return -1;

    IoApicRentry rentry = {.raw = IoApicRead64(ioapic->address, IOAPIC_RENTRY_BASE + 2 * isa_irq)};
    rentry.mask = 1;
    rentry.vector = vector;

    uint8_t polarity = flags & 0x3;
    if (polarity == 0b00 || polarity == 0b11) {
        rentry.polarity = IOAPIC_ACTIVE_LOW;
    } else if (polarity == 0b01) {
        rentry.polarity = IOAPIC_ACTIVE_HIGH;
    }

    uint8_t trigger_mode = (flags >> 2) & 0x3;
    if (trigger_mode == 0b00 || trigger_mode == 0b01) {
        rentry.trigger_mode = IOAPIC_EDGE;
    } else if (trigger_mode == 0b11) {
        rentry.trigger_mode = IOAPIC_LEVEL;
    }

    IoApicWrite64(ioapic->address, IOAPIC_RENTRY_BASE + 2 * isa_irq, rentry.raw);
    return 0;
}

int ApicSetIrqVector(uint8_t irq, uint8_t vector) {
    IoApic *ioapic = IoApicGetByIrq(irq);
    if (!ioapic)
        return -1;

    IoApicRentry rentry = {.raw = IoApicRead64(ioapic->address, IOAPIC_RENTRY_BASE + 2 * irq)};
    rentry.trigger_mode = IOAPIC_EDGE;
    rentry.vector = vector;
    IoApicWrite64(ioapic->address, IOAPIC_RENTRY_BASE + 2 * irq, rentry.raw);
    return 0;
}

int ApicSetIrqDest(uint8_t irq, uint8_t mode, uint8_t dest) {
    IoApic *ioapic = IoApicGetByIrq(irq);
    if (!ioapic)
        return -1;

    IoApicRentry rentry = {.raw = IoApicRead64(ioapic->address, IOAPIC_RENTRY_BASE + 2 * irq)};
    rentry.dest_mode = mode;
    rentry.dest = dest;
    IoApicWrite64(ioapic->address, IOAPIC_RENTRY_BASE + 2 * irq, rentry.raw);
    return 0;
}

int ApicSetIrqMask(uint8_t irq, uint8_t mask) {
    IoApic *ioapic = IoApicGetByIrq(irq);
    if (!ioapic)
        return -1;

    IoApicRentry rentry = {.raw = IoApicRead64(ioapic->address, IOAPIC_RENTRY_BASE + 2 * irq)};
    rentry.mask = mask & 1;
    IoApicWrite64(ioapic->address, IOAPIC_RENTRY_BASE + 2 * irq, rentry.raw);

    return 0;
}
