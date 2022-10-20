#include "acpi.h"

#include <cpu/intel.h>
#include <limine.h>
#include <utl/serial.h>

#include <dev/pci.h>

static volatile struct limine_rsdp_request rsdp_request = {
        .id = LIMINE_RSDP_REQUEST,
        .revision = 0,
};

static volatile struct limine_smbios_request smbios_request = {
        .id = LIMINE_SMBIOS_REQUEST,
        .revision = 0,
};


void AcpiInitialize(void) {
    AcpiRootSystemDescriptionPointer *rsdp = rsdp_request.response->address;

    ComPrint("[ACPI] RSDP: 0x%X, XSDT: 0x%X\n", rsdp, rsdp->xsdt_address);
    ComPrint("[ACPI] SMBIOS: 0x%X\n", smbios_request.response->entry_64);

    ComPrint("[ACPI] RSDP OEM: %c%c%c%c%c%c\n", rsdp->oem_id[0], rsdp->oem_id[1], rsdp->oem_id[2], rsdp->oem_id[3], rsdp->oem_id[4], rsdp->oem_id[5]);

    AcpiXsdt *xsdt = (AcpiXsdt *) rsdp->xsdt_address;

    AcpiMadt *madt = 0;
    AcpiFadt *fadt = 0;
    AcpiHpet *hpet = 0;
    AcpiMcfg *mcfg = 0;

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
        }
    }

    ComPrint("[ACPI] MADT: 0x%X, FADT: 0x%X, HPET: 0x%X, MCFG: 0x%X\n", madt, fadt, hpet, mcfg);

    PciInitialize(mcfg);
}