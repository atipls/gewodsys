#include "pci.h"

#include <cpu/intel.h>
#include <utl/serial.h>



static uint16_t PciReadWord(uint16_t bus, uint16_t device, uint16_t function, uint16_t offset) {
    uint32_t address = (uint32_t) (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xfc);
    IoOut32(0xcf8, address);
    return IoIn16(0xcfc + (offset & 2));
}

static uint32_t PciReadDword(uint16_t bus, uint16_t device, uint16_t function, uint16_t offset) {
    uint32_t address = (uint32_t) (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xfc);
    IoOut32(0xcf8, address);
    return IoIn32(0xcfc);
}

static const char *PciDeviceClasses[] = {
        "Unclassified",
        "Mass Storage Controller",
        "Network Controller",
        "Display Controller",
        "Multimedia Controller",
        "Memory Controller",
        "Bridge Device",
        "Simple Communication Controller",
        "Base System Peripheral",
        "Input Device Controller",
        "Docking Station",
        "Processor",
        "Serial Bus Controller",
        "Wireless Controller",
        "Intelligent Controller",
        "Satellite Communication Controller",
        "Encryption Controller",
        "Signal Processing Controller",
        "Processing Accelerator",
        "Non Essential Instrumentation"};

static const char *PciGetVendorName(uint16_t vendor_id) {
    switch (vendor_id) {
        case 0x1234: return "Bochs";
        case 0x8086: return "Intel";
        case 0x1022: return "AMD";
        case 0x10de: return "NVIDIA";
        case 0x1af4: return "Virtio";
        case 0x1b36: return "QEMU";
        case 0x1b4b: return "Red Hat";
        case 0x1b73: return "VMWare";
        default: return "Unknown";
    }
}

static const char *PciGetDeviceName(uint16_t vendor_id, uint16_t device_id) {
    switch (vendor_id) {
        case 0x8086:
            switch (device_id) {
                case 0x29C0: return "Xeon E3-1200 v2/3rd Gen Core processor DRAM Controller";
                case 0x10D3: return "7 Series/C210 Series Chipset Family 6-port SATA AHCI Controller";
                case 0x2918: return "82801IB (ICH9) LPC Interface Controller";
                case 0x2922: return "ICH9 SATA AHCI Controller driver";
                case 0x2930: return "82801I (ICH9 Family) SMBus Controller";
                case 0x24CD: return "82801DB/DBM (ICH4/ICH4-M) USB2 EHCI Controller";
                default:
                    ComPrint("Unknown Intel device: %x\n", device_id);
                    return "Unknown";
            }
        case 0x1234:
            switch (device_id) {
                case 0x1111: return "Bochs Graphics Adapter";
                default: return "Unknown";
            }
        default: return "Unknown";
    }
}

void PciInitialize(AcpiMcfg *mcfg) {
    // Enumerate the PCI devices.
    ComPrint("[ACPI] PCI Devices (%d - %d):\n", mcfg->start_bus_number, mcfg->end_bus_number);
    for (uint16_t bus = mcfg->start_bus_number; bus <= mcfg->end_bus_number; bus++) {
        for (uint16_t device = 0; device < 32; device++) {
            for (uint16_t function = 0; function < 8; function++) {
                uint16_t vendor_id = PciReadWord(bus, device, function, 0);
                if (vendor_id == 0xFFFF)
                    continue;

                uint16_t device_id = PciReadWord(bus, device, function, 2);
                uint16_t class_code = PciReadWord(bus, device, function, 10);

                uint8_t class = class_code >> 8;
                uint8_t subclass = (class_code >> 4) & 0xF;
                uint8_t prog_if = class_code & 0xF;

                ComPrint("[ACPI]   %d:%d:%d: %s %s [%s]\n", bus, device, function, PciGetVendorName(vendor_id), PciGetDeviceName(vendor_id, device_id), PciDeviceClasses[class]);
            }
        }
    }
}