#include "xhci.h"

#include <mem/memory.h>
#include <mem/paging.h>
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

    uint64_t mmio_base = MmMapToVirtual(mmio_base_physical, 0x4000);

    ComPrint("[XHCI]: MMIO base: 0x%X (Physical: 0x%X)\n", mmio_base, mmio_base_physical);

    XhciCapabilityRegisters *volatile reg_cap = (XhciCapabilityRegisters *volatile) mmio_base;
    XhciOperationalRegisters *volatile reg_op = (XhciOperationalRegisters *volatile) (mmio_base + reg_cap->length);

    ComPrint("[XHCI]: Capability length: %d bytes.\n", reg_cap->length);
    ComPrint("[XHCI]: USB Status: 0x%x\n", reg_op->usb_status);

    // Reset the controller. (Set Run/Stop to 0, then wait for it to be 0.)

    return;

    while (reg_op->usb_status & 0x80000000) {
        // Wait for the controller to be ready.

        ComPrint("[XHCI]: Waiting for controller to be ready... (%X)\n", reg_op->usb_status);
        uint16_t status_register = PciRead32(&driver->device, 0x4) >> 16;
        uint16_t command_register = PciRead32(&driver->device, 0x4) & 0xFFFF;

        ComPrint("[XHCI]: sts, cmd: %x, %x\n", status_register, command_register);
    }

    ComPrint("[XHCI]: Controller ready.\n");
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