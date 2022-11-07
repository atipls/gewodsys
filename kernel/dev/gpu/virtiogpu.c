#include "virtiogpu.h"


static uint8_t VirtioGpuTryProbe(PciDevice *device) {
    return device->vendor_id == 0x1AF4 && device->device_id == 0x1050;
}

static void VirtioGpuInitialize(PciDriver *driver) {


}

static void VirtioGpuFinalize(PciDriver *driver) {
    (void) driver;

    ComPrint("[VGPU] Finalizing...\n");
}

PciDriver kVirtioGpuDriver = {
        .name = "Virtio GPU Driver",
        .try_probe = VirtioGpuTryProbe,
        .initialize = VirtioGpuInitialize,
        .finalize = VirtioGpuFinalize,
};
