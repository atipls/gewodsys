#pragma once

#include <stdint.h>

#include "pci.h"

typedef struct __attribute__((packed))  {
    uint8_t length;
    uint8_t reserved_1;
    uint16_t hci_version;
    uint32_t hcs_params1;
    uint32_t hcs_params2;
    uint32_t hcs_params3;
    uint32_t hcc_params1;
    uint32_t doorbell_offset;
    uint32_t run_regs_off;
    uint32_t reserved_2[4];
} XhciCapabilityRegisters;

typedef struct __attribute__((packed))  {
    uint32_t usb_command;
    uint32_t usb_status;
    uint32_t page_size;
    uint32_t reserved_1;
    uint64_t device_notification;
    uint32_t reserved_2[2];
    uint32_t crcr_low;
    uint32_t crcr_high;
    uint32_t reserved_3[4];
    uint32_t dcbaap_low;
    uint32_t dcbaap_high;
    uint32_t config;
} XhciOperationalRegisters;

typedef struct __attribute__((packed))  {
    uint32_t port_status;
    uint32_t port_power;
    uint32_t port_link;
    uint32_t port_control;
} XhciPortRegisters;

typedef struct __attribute__((packed))  {
    uint32_t interrupter_status;
    uint32_t interrupter_enable;
    uint32_t interrupter_moderation;
    uint32_t interrupter_moderation_target;
    uint32_t interrupter_moderation_control;
    uint32_t reserved_1[11];
} XhciInterrupterRegisters;



extern PciDriver kXhciDriver;