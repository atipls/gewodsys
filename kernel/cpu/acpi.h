#pragma once

#include <stdint.h>


typedef struct __attribute__((packed)) {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} AcpiRootSystemDescriptionPointer;

typedef struct __attribute__((packed)) {
    uint32_t signature;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} AcpiTableHeader;

typedef struct __attribute__((packed)) {
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved;
    uint64_t address;
} AcpiGenericAddressStructure;

typedef struct __attribute__((packed)) {
    AcpiTableHeader header;
    uint64_t pointers[];
} AcpiXsdt;

typedef struct __attribute__((packed)) {
    AcpiTableHeader header;
    uint32_t local_apic_address;
    uint32_t flags;
} AcpiMadt;

typedef struct __attribute__((packed)) {
    AcpiTableHeader header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferred_pm_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pm_tmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t pm1_evt_len;
    uint8_t pm1_cnt_len;
    uint8_t pm2_cnt_len;
    uint8_t pm_tmr_len;
    uint8_t gpe0_blk_len;
    uint8_t gpe1_blk_len;
    uint8_t gpe1_base;
    uint8_t cst_cnt;
    uint16_t p_lvl2_lat;
    uint16_t p_lvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alrm;
    uint8_t mon_alrm;
    uint8_t century;
    uint16_t iapc_boot_arch;
    uint8_t reserved2;
    uint32_t flags;
    AcpiGenericAddressStructure reset_reg;
    uint8_t reset_value;
    uint8_t reserved3[3];
    uint64_t x_firmware_ctrl;
    uint64_t x_dsdt;
    AcpiGenericAddressStructure x_pm1a_evt_blk;
    AcpiGenericAddressStructure x_pm1b_evt_blk;
    AcpiGenericAddressStructure x_pm1a_cnt_blk;
    AcpiGenericAddressStructure x_pm1b_cnt_blk;
    AcpiGenericAddressStructure x_pm2_cnt_blk;
    AcpiGenericAddressStructure x_pm_tmr_blk;
    AcpiGenericAddressStructure x_gpe0_blk;
    AcpiGenericAddressStructure x_gpe1_blk;
} AcpiFadt;

typedef struct __attribute__((packed)) {
    AcpiTableHeader header;
    uint32_t hardware_rev_id;
    uint32_t comparator_count : 5;
    uint32_t counter_size : 1;
    uint32_t reserved : 1;
    uint32_t legacy_replacement : 1;
    uint32_t pci_vendor_id : 16;
    AcpiGenericAddressStructure address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} AcpiHpet;

typedef struct __attribute__((packed)) {
    AcpiTableHeader header;
    uint64_t reserved;
    uint64_t base_address;
    uint16_t pci_segment_group;
    uint8_t start_bus_number;
    uint8_t end_bus_number;
    uint32_t reserved2;
} AcpiMcfg;

#define ACPI_MADT_ID 0x43495041 /* "APIC" */
#define ACPI_FADT_ID 0x50434146 /* "FACP" */
#define ACPI_HPET_ID 0x54455048 /* "HPET" */
#define ACPI_MCFG_ID 0x4746434D /* "MCFG" */

void AcpiInitialize(void);