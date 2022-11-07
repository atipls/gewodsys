#include "acpi.h"

#include <cpu/intel.h>
#include <limine.h>
#include <utl/serial.h>

#include <dev/pci.h>

#include <mem/heap.h>
#include <mem/vmm.h>
#include <stddef.h> // For LAI

#include <cpu/lai/core/core.h>

static volatile struct limine_rsdp_request rsdp_request = {
        .id = LIMINE_RSDP_REQUEST,
        .revision = 0,
};

static volatile struct limine_smbios_request smbios_request = {
        .id = LIMINE_SMBIOS_REQUEST,
        .revision = 0,
};


void AcpiInitialize(void) {
    ComPrint("Sex: 0x%X\n", kmalloc(0x1000));
    AcpiRootSystemDescriptionPointer *rsdp = rsdp_request.response->address;

    ComPrint("[ACPI] RSDP: 0x%X, XSDT: 0x%X\n", rsdp, rsdp->xsdt_address);
    ComPrint("[ACPI] SMBIOS: 0x%X\n", smbios_request.response->entry_64);

    ComPrint("[ACPI] RSDP OEM: %c%c%c%c%c%c\n", rsdp->oem_id[0], rsdp->oem_id[1], rsdp->oem_id[2], rsdp->oem_id[3], rsdp->oem_id[4], rsdp->oem_id[5]);

    AcpiXsdt *xsdt = (AcpiXsdt *) rsdp->xsdt_address;

    AcpiMadt *madt = 0;
    AcpiFadt *fadt = 0;
    AcpiHpet *hpet = 0;
    AcpiMcfg *mcfg = 0;
    AcpiSsdt *ssdt = 0;

    int length = (xsdt->header.length - sizeof(AcpiTableHeader)) / sizeof(uint64_t);
    ComPrint("[ACPI] Available tables (%d total) (%d):\n", length, xsdt->header.length);

    for (int i = 0; i < length; i++) {
        AcpiTableHeader *header = (AcpiTableHeader *) xsdt->pointers[i];
        ComPrint("[ACPI]   0x%X: %c%c%c%c\n", header, header->signature & 0xFF, (header->signature >> 8) & 0xFF, (header->signature >> 16) & 0xFF, (header->signature >> 24) & 0xFF);

        switch (header->signature) {
            case ACPI_MADT_ID: madt = (AcpiMadt *) header; break;
            case ACPI_FADT_ID: fadt = (AcpiFadt *) header; break;
            case ACPI_HPET_ID: hpet = (AcpiHpet *) header; break;
            case ACPI_MCFG_ID: mcfg = (AcpiMcfg *) header; break;
            case ACPI_SSDT_ID: ssdt = (AcpiSsdt *) header; break;
        }
    }

    ComPrint("[ACPI] MADT: 0x%X, FADT: 0x%X, HPET: 0x%X, MCFG: 0x%X SSDT: 0x%X\n", madt, fadt, hpet, mcfg, ssdt);

    //lai_enable_tracing(LAI_TRACE_NS);
    lai_set_acpi_revision(rsdp->revision);
    lai_create_namespace();

    lai_enable_acpi(1);

    /*
    struct lai_ns_iterator it;
    lai_nsnode_t *node = NULL;
    while ((node = lai_ns_iterate(&it))) {
        ComPrint("[ACPI] Node: %s\n", lai_stringify_node_path(node));
    }
    */

    PciInitialize(mcfg);
}

// Required stuff for LAI:

void laihost_log(int level, const char *msg) {
    ComPrint("[LAI-%d] %s\n", level, msg);
}

__attribute__((noreturn)) void laihost_panic(const char *msg) {
    ComPrint("[LAI-PANIC] %s\n", msg);
    __asm__ volatile("cli; hlt");
    while (1)
        ;
}

void *laihost_malloc(size_t size) {
    return kmalloc(size);
}

void laihost_free(void *ptr, size_t size) {
    (void) size;
    kfree(ptr);
}

void *laihost_realloc(void *ptr, size_t size, size_t old_size) {
    if (size == old_size)
        return ptr;

    if (size == 0) {
        kfree(ptr);
        return 0;
    }

    if (!ptr)
        return kmalloc(size);

    void *new_ptr = kmalloc(size);
    RtCopyMemory(new_ptr, ptr, old_size);
    kfree(ptr);

    return new_ptr;
}

void *laihost_map(size_t address, size_t count) {
    address &= ~0xFFF;
    for (size_t i = 0; i < count; i += PAGE_SIZE)
        MmMapMemory((void *) address + i, (void *) address + i);
    return (void *) address;
}

void laihost_unmap(void *address, size_t count) {
    ComPrint("[LAI] Unmapping is not supported!\n");
}

void *laihost_scan(const char *sig, size_t index) {
    AcpiRootSystemDescriptionPointer *rsdp = rsdp_request.response->address;
    AcpiXsdt *xsdt = (AcpiXsdt *) rsdp->xsdt_address;

    ComPrint("[LAI] Scanning for %s (%d)\n", sig, index);

    int length = (xsdt->header.length - sizeof(AcpiTableHeader)) / sizeof(uint64_t);
    for (int i = 0; i < length; i++) {
        AcpiTableHeader *header = (AcpiTableHeader *) xsdt->pointers[i];
        if (header->signature == *(uint32_t *) sig) {
            if (index == 0)
                return header;
            index--;
        }
    }

    if (*(uint32_t *) sig == ACPI_DSDT_ID) {
        for (int i = 0; i < length; i++) {
            AcpiTableHeader *header = (AcpiTableHeader *) xsdt->pointers[i];
            if (header->signature == ACPI_FADT_ID) {
                AcpiFadt *fadt = (AcpiFadt *) header;
                return (void *) fadt->x_dsdt;
            }
        }
    }

    return 0;
}

void laihost_outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1"
                     :
                     : "a"(value), "Nd"(port));
}

void laihost_outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1"
                     :
                     : "a"(value), "Nd"(port));
}

void laihost_outd(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1"
                     :
                     : "a"(value), "Nd"(port));
}

uint8_t laihost_inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0"
                     : "=a"(ret)
                     : "Nd"(port));
    return ret;
}

uint16_t laihost_inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0"
                     : "=a"(ret)
                     : "Nd"(port));
    return ret;
}

uint32_t laihost_ind(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0"
                     : "=a"(ret)
                     : "Nd"(port));
    return ret;
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
    ComPrint("[LAI] PCI SEG: %d\n", seg);
    PciDevice device = {.bus = bus, .device = slot, .function = func};
    return PciRead8(&device, offset);
}

uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
    ComPrint("[LAI] PCI SEG: %d\n", seg);
    PciDevice device = {.bus = bus, .device = slot, .function = func};
    return PciRead16(&device, offset);
}

uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset) {
    ComPrint("[LAI] PCI SEG: %d\n", seg);
    PciDevice device = {.bus = bus, .device = slot, .function = func};
    return PciRead32(&device, offset);
}

void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset, uint8_t value) {
    ComPrint("[LAI] PCI SEG: %d\n", seg);
    PciDevice device = {.bus = bus, .device = slot, .function = func};
    PciWrite8(&device, offset, value);
}

void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset, uint16_t value) {
    ComPrint("[LAI] PCI SEG: %d\n", seg);
    PciDevice device = {.bus = bus, .device = slot, .function = func};
    PciWrite16(&device, offset, value);
}

void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t func, uint16_t offset, uint32_t value) {
    ComPrint("[LAI] PCI SEG: %d\n", seg);
    PciDevice device = {.bus = bus, .device = slot, .function = func};
    PciWrite32(&device, offset, value);
}

void laihost_sleep(uint64_t ms) {
    ComPrint("[LAI] Sleep is not supported!\n");
}