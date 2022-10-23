#include "xhci.h"

#include <utl/serial.h>

static uint8_t XhciTryProbe(PciDevice *device) {
    uint8_t is_qemu_pci = device->vendor_id == 0x1B36 && device->device_id == 0x000D;
    uint8_t is_intl_pci = device->vendor_id == 0x8086 && device->device_id == 0x22B5;

    return is_qemu_pci || is_intl_pci;
}

static void XhciInitialize(PciDriver *driver) {
    // Configure the PCI controls.
    PciMaybeEnableBusMastering(&driver->device);
    PciMaybeEnableMemoryAccess(&driver->device);

    uint64_t mmio_base_physical = PciRead32(&driver->device, 0x10);
    if (mmio_base_physical & kPciBar64)
        mmio_base_physical |= (uint64_t) PciRead32(&driver->device, 0x14) << 32;
    mmio_base_physical &= ~0xF;

    uint64_t mmio_base = MmMapToVirtual(mmio_base_physical, 0x1000);

    ComPrint("[XHCI]: MMIO base: %X\n", mmio_base);
    ComPrint("[XHCI]: Reading: %X\n", *(uint32_t *) mmio_base);
}

static void XhciFinalize(PciDriver *driver) {
    ComPrint("[XHCI]: Finalizing...\n");
}

PciDriver kXhciDriver = {
        .name = "USB xHCI Controller",
        .try_probe = XhciTryProbe,
        .initialize = XhciInitialize,
        .finalize = XhciFinalize,
};