#pragma once

#include <stdint.h>
#include <cpu/acpi.h>

typedef struct PciBar {
    uint64_t base;
    uint64_t size;
    uint8_t flags;
    uint8_t type;
} PciBar;

enum PciBarFlags {
    kPciBarIo = 0x01,
    kPciBarLowMem = 0x02,
    kPciBar64 = 0x04,
    kPciBarPrefetch = 0x08,
};

typedef struct PciDevice {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass_code;
    uint8_t interface_code;

    PciBar bars[6];
} PciDevice;

typedef struct PciDriver {
    const char *name;
    PciDevice device;

    void *data;

    uint8_t (*try_probe)(PciDevice *);
    void (*initialize)(struct PciDriver *);
    void (*finalize)(struct PciDriver *);
} PciDriver;

extern PciDriver *kPciDrivers[];

uint8_t PciRead8(PciDevice *device, uint8_t offset);
uint16_t PciRead16(PciDevice *device, uint8_t offset);
uint32_t PciRead32(PciDevice *device, uint8_t offset);

void PciWrite8(PciDevice *device, uint8_t offset, uint8_t value);
void PciWrite16(PciDevice *device, uint8_t offset, uint16_t value);
void PciWrite32(PciDevice *device, uint8_t offset, uint32_t value);

void PciInitialize(AcpiMcfg *mcfg);

void PciMaybeEnableBusMastering(PciDevice *device);
void PciMaybeEnableMemoryAccess(PciDevice *device);

void PciEnableMsiVector(PciDevice *device, uint8_t index, uint8_t vector);
void PciDisableMsiVector(PciDevice *device, uint8_t index);