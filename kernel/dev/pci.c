#include "pci.h"

#include <cpu/intel.h>
#include <utl/serial.h>

#include "gpu/cherrytrail.h"
#include "usb/xhci.h"

PciDriver *kPciDrivers[] = {
        &kXhciDriver,
        &kCherryTrailDriver,
};

PciDriver kLoadedPciDrivers[256];
uint8_t kLoadedPciDriverCount = 0;

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

uint8_t PciRead8(PciDevice *device, uint8_t offset) {
    uint32_t address = (1 << 31) | (device->bus << 16) | (device->device << 11) | (device->function << 8) | (offset & 0xFC);
    IoOut32(PCI_CONFIG_ADDRESS, address);
    return IoIn8(PCI_CONFIG_DATA + (offset & 0x3));
}

uint16_t PciRead16(PciDevice *device, uint8_t offset) {
    uint32_t address = (1 << 31) | (device->bus << 16) | (device->device << 11) | (device->function << 8) | (offset & 0xFC);
    IoOut32(PCI_CONFIG_ADDRESS, address);
    return (uint16_t) ((IoIn32(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xffff);
}

uint32_t PciRead32(PciDevice *device, uint8_t offset) {
    uint16_t low = PciRead16(device, offset);
    uint16_t high = PciRead16(device, offset + 2);
    return (high << 16) | low;
}

void PciWrite8(PciDevice *device, uint8_t offset, uint8_t value) {
    uint32_t address = (1 << 31) | (device->bus << 16) | (device->device << 11) | (device->function << 8) | (offset & 0xFC);
    IoOut32(PCI_CONFIG_ADDRESS, address);
    IoOut8(PCI_CONFIG_DATA + (offset & 0x3), value);
}

void PciWrite16(PciDevice *device, uint8_t offset, uint16_t value) {
    uint32_t address = (1 << 31) | (device->bus << 16) | (device->device << 11) | (device->function << 8) | (offset & 0xFC);
    IoOut32(PCI_CONFIG_ADDRESS, address);
    IoOut16(PCI_CONFIG_DATA + (offset & 0x2), value);
}

void PciWrite32(PciDevice *device, uint8_t offset, uint32_t value) {
    PciWrite16(device, offset, (uint16_t) (value & 0xFFFF));
    PciWrite16(device, offset + 2, (uint16_t) (value >> 16) & 0xFFFF);
}

void PciGetBar(PciDevice *device, uint32_t bar, uint64_t *address) {
    uint16_t bar_address = 0x10 + (bar * 4);
    uint32_t bar_value = PciRead32(device, bar_address);

    if (bar_value & 0x1) {
        *address = bar_value & 0xFFFFFFFC;
    } else {
        *address = bar_value & 0xFFFFFFF0;
    }

    if (bar_value & 0x4) {
        uint32_t bar_value_upper = PciRead32(device, bar_address + 4);
        *address |= ((uint64_t) bar_value_upper) << 32;
    }

    ComPrint("[PCI] BAR %d: %X", bar, *address);
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
        "Non Essential Instrumentation",
};

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
                case 0x2280: return "Atom SoC Transaction Register";
                case 0x22B0: return "Atom Integrated Graphics Controller";
                case 0x22B8: return "Atom Imaging Unit";
                case 0x22D8: return "Atom Integrated Sensor Hub";
                case 0x22DC: return "Atom Power Management Controller";
                case 0x22B5: return "Atom USB xHCI Controller";
                case 0x2298: return "Atom Trusted Execution Engine";
                case 0x22C8: return "Atom PCI Express Port #1";
                case 0x229C: return "Atom Package Control Unit";
                default:
                    ComPrint("Unknown Intel device: %x\n", device_id);
                    return "Unknown";
            }
        case 0x1234:
            switch (device_id) {
                case 0x1111: return "Graphics Adapter";
                default:
                    ComPrint("Unknown bochs device: %x\n", device_id);
                    return "Unknown";
            }
        case 0x1b36:
            switch (device_id) {
                case 0x000D: return "XHCI Host Controller";
                default:
                    ComPrint("Unknown qemu device: %x\n", device_id);
                    return "Unknown";
            }
        case 0x1af4:
            switch (device_id) {
                case 0x1000: return "Network Adapter";
                case 0x1001: return "Block Device";
                case 0x1002: return "Console";
                case 0x1041: return "RNG";
                case 0x1050: return "GPU";
                default:
                    ComPrint("Unknown virtio device: %x\n", device_id);
                    return "Unknown";
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
                PciDevice pci_device = {
                        .bus = bus,
                        .device = device,
                        .function = function,
                };

                pci_device.vendor_id = PciRead16(&pci_device, 0);
                if (pci_device.vendor_id == 0xFFFF)
                    continue;

                pci_device.device_id = PciRead16(&pci_device, 2);
                uint16_t class_code = PciRead16(&pci_device, 10);

                pci_device.class_code = class_code >> 8;
                pci_device.subclass_code = (class_code >> 4) & 0xF;
                pci_device.interface_code = class_code & 0xF;

                ComPrint("[ACPI]   %d:%d:%d: %s %s [%s]\n",
                         pci_device.bus, pci_device.device, pci_device.function,
                         PciGetVendorName(pci_device.vendor_id),
                         PciGetDeviceName(pci_device.vendor_id, pci_device.device_id),
                         PciDeviceClasses[pci_device.class_code]);

                for (uint64_t i = 0; i < sizeof(kPciDrivers) / sizeof(PciDriver *); i++) {
                    PciDriver *driver = kPciDrivers[i];
                    if (!driver->try_probe(&pci_device))
                        continue;
                    ComPrint("[ACPI]    Loading driver %s\n", driver->name);
                    kLoadedPciDrivers[kLoadedPciDriverCount++] = *driver;

                    PciDriver *loaded_driver = &kLoadedPciDrivers[kLoadedPciDriverCount - 1];
                    loaded_driver->device = pci_device;
                    loaded_driver->initialize(loaded_driver);

                    break;
                }
            }
        }
    }
}

void PciMaybeEnableBusMastering(PciDevice *device) {
    uint16_t command = PciRead16(device, 4);
    if (command & (1 << 2))
        return;

    command |= (1 << 2);
    PciWrite16(device, 4, command);
}

void PciMaybeEnableMemoryAccess(PciDevice *device) {
    uint16_t command = PciRead16(device, 4);
    if (command & (1 << 1))
        return;

    command |= (1 << 1);
    PciWrite16(device, 4, command);
}

void PciEnableMsiVector(PciDevice *device, uint8_t index, uint8_t vector) {

}

void PciDisableMsiVector(PciDevice *device, uint8_t index) {

}

/*


void PciEnableMsiVector(pcie_device_t *device, uint8_t index, uint8_t vector) {
  pcie_cap_msix_t *msix_cap = pcie_get_cap(device, PCI_CAP_MSIX);
  if (msix_cap == NULL) {
    panic("[pcie] could not locate msix structure");
  }

// kprintf("pcie: msi index = %d\n", index);
// kprintf("pcie: message control = %#b\n", ((pci_cap_msix_t *)((void *)msix_cap))->msg_ctrl);
// kprintf("pcie: table size = %d\n", msix_cap->tbl_sz);

uint16_t tbl_size = msix_cap->tbl_sz + 1;
kassert(index < tbl_size);

pcie_bar_t *bar = pcie_get_bar(device, msix_cap->bir);
if (bar->virt_addr == 0) {
    panic("[pcie] msix memory space not mapped");
}

pcie_msix_entry_t *table = (void *)(bar->virt_addr + (msix_cap->tbl_ofst << 3));
pcie_msix_entry_t *entry = &table[index];

entry->msg_addr = msi_msg_addr(PERCPU_ID);
entry->msg_data = msi_msg_data(vector, 1, 0);
entry->masked = 0;

msix_cap->en = 1;
}

void pcie_disable_msi_vector(pcie_device_t *device, uint8_t index) {
    pcie_cap_msix_t *msix_cap = pcie_get_cap(device, PCI_CAP_MSIX);
    if (msix_cap == NULL) {
        panic("[pcie] could not locate msix structure");
    }

    uint16_t tbl_size = msix_cap->tbl_sz + 1;
    kassert(index < tbl_size);

    pcie_bar_t *bar = pcie_get_bar(device, msix_cap->bir);
    if (bar->virt_addr == 0) {
        panic("[pcie] msix memory space not mapped");
    }

    pcie_msix_entry_t *table = (void *)(bar->virt_addr + (msix_cap->tbl_ofst << 3));
    table[index].masked = 1;
}


 */