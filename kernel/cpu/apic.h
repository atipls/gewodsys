#pragma once

#include <dev/pci.h>
#include <stdint.h>

#include "acpi.h"

typedef void (*IrqHandlerFn)(uint8_t, void *);

void ApicInitialize(AcpiMadt *madt);
void ApicInitializeInterrupts();

int ApicGetHighestIrq();

int ApicSetIrqIsaRouting(uint8_t isa_irq, uint8_t vector, uint16_t flags);
int ApicSetIrqVector(uint8_t irq, uint8_t vector);
int ApicSetIrqDest(uint8_t irq, uint8_t mode, uint8_t dest);
int ApicSetIrqMask(uint8_t irq, uint8_t mask);

int ApicEnableInterrupt(uint8_t irq);
int ApicDisableInterrupt(uint8_t irq);

int ApicAllocateSoftwareIrq();

void ApicRegisterIrqHandler(uint8_t irq, IrqHandlerFn handler, void *data);

int ApicEnableMsiInterrupt(uint8_t irq, uint8_t index, PciDevice *device);
int ApicDisableMsiInterrupt(uint8_t irq, uint8_t index, PciDevice *device);