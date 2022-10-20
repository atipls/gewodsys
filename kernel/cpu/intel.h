#pragma once
#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint16_t base_low16;
    uint8_t base_mid8;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high8;
} GlobalDescriptorTableEntry;

typedef struct __attribute__((packed)) {
    uint16_t length;
    uint16_t base_low16;
    uint8_t base_mid8;
    uint8_t flags0;
    uint8_t flags1;
    uint8_t base_high8;
    uint32_t base_upper32;
    uint32_t reserved;
} TaskStateSegmentEntry;

typedef struct __attribute__((packed)) {
    uint32_t reserved0;
    uint64_t rsp[3];
    uint64_t reserved1;
    uint64_t ist[7];
    uint32_t reserved2;
    uint32_t reserved3;
    uint16_t reserved4;
    uint16_t iopb_offset;
} TaskStateSegment;

typedef struct __attribute__((packed)) {
    GlobalDescriptorTableEntry null;
    GlobalDescriptorTableEntry code16;
    GlobalDescriptorTableEntry data16;
    GlobalDescriptorTableEntry code32;
    GlobalDescriptorTableEntry data32;
    GlobalDescriptorTableEntry code64;
    GlobalDescriptorTableEntry data64;
    GlobalDescriptorTableEntry user_data;
    GlobalDescriptorTableEntry user_code;
    TaskStateSegmentEntry tss;
} GlobalDescriptorTable;

typedef struct __attribute__((packed)) {
    uint16_t size;
    void* offset;
} GlobalDescriptorTableDescriptor;

typedef struct __attribute__((packed)) {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t ist;
    uint8_t attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} InterruptDescriptor;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    void *base;
} InterruptTableDescriptor;

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01


void IntelInitialize(uint64_t kernel_stack);
void IntelRemapPic(void);

uint8_t IoIn8(uint16_t port);
uint16_t IoIn16(uint16_t port);
uint32_t IoIn32(uint16_t port);

void IoOut8(uint16_t port, uint8_t val);
void IoOut16(uint16_t port, uint16_t val);
void IoOut32(uint16_t port, uint32_t val);

void IoWait(void);