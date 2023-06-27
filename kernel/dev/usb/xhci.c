#include "xhci.h"

#include <mem/vmm.h>
#include <utl/serial.h>

#define XCAP_ID(v) (((uint32_t) (v)) & 0xFF)
#define XCAP_NEXT(v) (((((uint32_t) (v)) >> 8) & 0xFF) << 2)

typedef union {
    uint32_t raw;
    struct {
        uint32_t bir : 3;
        uint32_t off : 29;
    };
} XhciCapMsix;

typedef volatile struct {
    uint64_t msg_addr;
    uint32_t msg_data;
    uint32_t masked : 1;
    uint32_t : 31;
} XhciMsixEntry;

static void XhciHostIrq(uint8_t irq, void *data);

static uint8_t XhciTryProbe(PciDevice *device) {
    uint8_t is_qemu_pci = device->vendor_id == 0x1B36 && device->device_id == 0x000D;
    uint8_t is_intl_pci = device->vendor_id == 0x8086 && device->device_id == 0x22B5;

    return is_qemu_pci || is_intl_pci;
}

static void XhciInitialize(PciDriver *driver) {
    // Configure the PCI controls.
    PciMaybeEnableBusMastering(&driver->device);
    PciMaybeEnableMemoryAccess(&driver->device);

    uint64_t mmio_base = PciRead32(&driver->device, 0x10);
    if (mmio_base & kPciBar64)
        mmio_base |= (uint64_t) PciRead32(&driver->device, 0x14) << 32;
    mmio_base &= ~0xF;

    MmMapMemory((void *) mmio_base, (void *) mmio_base);

    XhciDevice *xhci = driver->data = (XhciDevice *) kmalloc(sizeof(XhciDevice));

    xhci->pci = &driver->device;

    xhci->mmio = (void *) mmio_base;

    xhci->cap = (XhciCapabilityRegisters *) mmio_base;
    xhci->op = (XhciOperationalRegisters *) (mmio_base + xhci->cap->length);
    xhci->rt = (XhciRuntimeRegisters *) (mmio_base + xhci->cap->run_regs_off);
    xhci->db = (XhciDoorbellRegister *) (mmio_base + xhci->cap->doorbell_offset);
    xhci->xcap = (void *) (mmio_base + ((((xhci->cap->hcc_params1) >> 16) & 0xFFF) << 2));

    ComPrint("[XHCI] XCAP: 0x%X\n", xhci->xcap);

    // Reset the controller.
    xhci->op->usb_command &= ~kUsbCmdRun;
    xhci->op->usb_command |= kUsbCmdHcReset;
    while (xhci->op->usb_status & kUsbStsControllerNotReady)
        ComPrint("[XHCI] Waiting for controller to be ready... (%X)\n", xhci->op->usb_status);

    ComPrint("[XHCI] Controller ready.\n");

    // Enable every slot.
    xhci->op->config = xhci->cap->hcc_params1 & 0xFF;

    // Set up the device context base address array pointer.
    uint64_t dcbaap = kmalloc((xhci->cap->hcs_params1 & 0xFF) * sizeof(uint64_t));
    xhci->dcbaap = MmGetPhysicalAddress((void *) dcbaap);

    xhci->op->dcbaap_low = xhci->dcbaap;
    xhci->op->dcbaap_high = xhci->dcbaap >> 32;

    // Set up the command ring.
    xhci->cmd = XhciRingCreate(CMD_RING_SIZE);

    xhci->interrupter_bitmap_size = (xhci->cap->hcs_params1 >> 8) & 0x7FF;
    ComPrint("[XHCI] Interrupter bitmap size: %d\n", xhci->interrupter_bitmap_size);
    xhci->interrupter_bitmap = (uint8_t *) kmalloc(xhci->interrupter_bitmap_size / 8);
    RtZeroMemory(xhci->interrupter_bitmap, xhci->interrupter_bitmap_size / 8);

    xhci->interrupter = XhciInterrupterCreate(xhci, XhciHostIrq, xhci);
}

static void XhciFinalize(PciDriver *driver) {
    (void) driver;

    ComPrint("[XHCI] Finalizing...\n");
}

XhciRing *XhciRingCreate(uint64_t size) {
    uint32_t num_pages = ((size * sizeof(XhciTrb)) + 0xFFF) / PAGE_SIZE;
    uint32_t num_trbs = num_pages * PAGE_SIZE / sizeof(XhciTrb);

    XhciRing *ring = (XhciRing *) kmalloc(sizeof(XhciRing));
    RtZeroMemory(ring, sizeof(XhciRing));

    ring->ptr = kmalloc(size * sizeof(XhciTrb));
    ring->index = 0;
    ring->max_index = num_trbs;
    ring->cycle = 1;

    return ring;
}

void XhciRingDestroy(XhciRing *ring) {
    kfree(ring->ptr);
    kfree(ring);
}

int XhciRingAdd(XhciRing *ring, XhciTrb *trb) {
    if (trb->trb_type == 0)
        return -1;

    trb->cycle = ring->cycle;
    ring->ptr[ring->index] = *trb;
    ring->index++;

    if (ring->index == ring->max_index - 1) {
        XhciTrbLink link = {0};
        link.trb_type = kTrbLink;
        link.cycle = ring->cycle;
        link.toggle_cycle = 1;
        link.rs_addr = MmGetPhysicalAddress((void *) ring->ptr);

        ring->ptr[ring->index] = *(XhciTrb *) &link;
        ring->index = 0;
        ring->cycle = !ring->cycle;
    }

    return 0;
}

int XhciRingGet(XhciRing *ring, XhciTrb *trb) {
    if (!trb || ring->index == 0)
        return -1;

    XhciTrb *last = &ring->ptr[ring->index];
    if (last->trb_type == 0)
        return -1;

    ring->index++;

    if (ring->index == ring->max_index) {
        ring->index = 0;
        ring->cycle = !ring->cycle;
    }

    *trb = *last;
    return 0;
}

uint64_t XhciRingGetPhysicalAddress(XhciRing *ring) {
    return MmGetPhysicalAddress(ring->ptr) + (ring->index * sizeof(XhciTrb));
}

uint64_t XhciRingSize(XhciRing *ring) {
    return ring->max_index * sizeof(XhciTrb);
}

XhciInterrupter *XhciInterrupterCreate(XhciDevice *xhci, IrqHandlerFn handler, void *data) {
    // Find a free interrupter.
    int interrupter_index = -1;
    for (int i = 0; i < xhci->interrupter_bitmap_size; i++) {
        if (!INTERRUPTER_BITMAP_GET(xhci, i)) {
            interrupter_index = i;
            break;
        }
    }

    if (interrupter_index == -1)
        return 0;

    INTERRUPTER_BITMAP_SET(xhci, interrupter_index);

    int irq = ApicAllocateSoftwareIrq();
    ComPrint("[XHCI] Allocated IRQ %d for interrupter %d\n", irq, interrupter_index);
    if (irq == -1)
        return 0;

    ApicRegisterIrqHandler(irq, handler, data);
    ApicEnableInterrupt(irq);

    uintptr_t msix = 0;

    uint32_t capabilities_address = PciRead32(xhci->pci, 0x34) & 0xFF;
    uint32_t capability = PciRead32(xhci->pci, capabilities_address);
    while (1) {
        // We found the MSI-X capability.
        if (XCAP_ID(capability) == 0x11) {
            XhciCapMsix msix_capability = {.raw = PciRead32(xhci->pci, capabilities_address + 0x4)};
            if (msix_capability.bir != 0)
                ComPrint("[XHCI] MSI-X BIR is not 0! (%d). This will blow up.\n", msix_capability.bir);

            msix = (uintptr_t) (xhci->mmio + msix_capability.off);

            // Enable MSI-X.
            capability |= 0x8000;
            PciWrite32(xhci->pci, capabilities_address, capability);
            break;
        }

        if (XCAP_NEXT(capability) == 0)
            break;

        capabilities_address += XCAP_NEXT(capability);
        capability = PciRead32(xhci->pci, capabilities_address);
    }

    if (!msix) {
        ComPrint("[XHCI] Could not find MSI-X capability!\n");
        return 0;
    }

    // Set up the MSI-X table entry.
    XhciMsixEntry *table = (XhciMsixEntry *) msix;
    XhciMsixEntry *entry = &table[interrupter_index];

    entry->msg_addr = 0xFEE00000 | (0 << 12); // Hardcoded to CPU0.
    entry->msg_data = irq;
    entry->masked = 0;


    XhciErstEntry *erst = (XhciErstEntry *) kmalloc(ERST_SIZE * sizeof(XhciErstEntry));
    XhciRing *ring = XhciRingCreate(EVT_RING_SIZE);

    erst->rs_addr = XhciRingGetPhysicalAddress(ring);
    erst->rs_size = XhciRingSize(ring);

    XhciInterrupter *interrupter = (XhciInterrupter *) kmalloc(sizeof(XhciInterrupter));

    interrupter->index = interrupter_index;
    interrupter->vector = irq;
    interrupter->ring = ring;
    interrupter->erst = erst;

    return interrupter;
}

static void XhciHostIrq(uint8_t irq, void *data) {
    ComPrint("[XHCI] Host IRQ!\n");

    XhciDevice *xhci = (XhciDevice *) data;

    xhci->op->usb_status |= kUsbStsEventInterrupt;
    xhci->rt->interrupters[0].iman_r |= kImanInterruptPending;

    if (xhci->op->usb_status & kUsbStsHcError)
        ComPrint("[XHCI] Host controller error!\n");
    if (xhci->op->usb_status & kUsbStsHostSystemError)
        ComPrint("[XHCI] Host system error!\n");

    // TODO: Signal the event ring.
}

PciDriver kXhciDriver = {
        .name = "USB xHCI Controller",
        .try_probe = XhciTryProbe,
        .initialize = XhciInitialize,
        .finalize = XhciFinalize,
};