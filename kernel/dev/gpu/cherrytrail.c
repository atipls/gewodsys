#include "cherrytrail.h"


static uint8_t CherryTrailTryProbe(PciDevice *device) {
    return device->vendor_id == 0x8086 && device->device_id == 0x22B0;
}

static void CherryTrailInitialize(PciDriver *driver) {
    uint64_t gttmmaddr = PciRead32(&driver->device, 0x10);
    if (gttmmaddr & kPciBar64)
        gttmmaddr |= (uint64_t) PciRead32(&driver->device, 0x14) << 32;
    gttmmaddr &= ~0xF;

    uint64_t gmadr = PciRead32(&driver->device, 0x18);
    if (gmadr & kPciBar64)
        gmadr |= (uint64_t) PciRead32(&driver->device, 0x1C) << 32;
    gmadr &= ~0xF;

    uint32_t iobase = PciRead32(&driver->device, 0x20) & 0xFFFFFFFC;

    ComPrint("[CHTR] GTTMMADDR: 0x%X\n", gttmmaddr);
    ComPrint("[CHTR] GMADR: 0x%X\n", gmadr);
    ComPrint("[CHTR] IOBASE: 0x%X\n", iobase);

    MmMapMemory((void *) gttmmaddr, (void *) gttmmaddr);
    MmMapMemory((void *) gmadr, (void *) gmadr);
}

static void CherryTrailFinalize(PciDriver *driver) {
    (void) driver;
}

PciDriver kCherryTrailDriver = {
        .name = "CherryTrail GPU",
        .try_probe = CherryTrailTryProbe,
        .initialize = CherryTrailInitialize,
        .finalize = CherryTrailFinalize,
};