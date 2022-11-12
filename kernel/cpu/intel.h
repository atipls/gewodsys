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
    void *offset;
} GlobalDescriptorTableDescriptor;

typedef struct __attribute__((packed)) {
    uint64_t offset0 : 16;
    uint64_t selector : 16;
    uint64_t ist : 3;
    uint64_t : 5;
    uint64_t type : 4;
    uint64_t : 1;
    uint64_t dpl : 2;
    uint64_t present : 1;
    uint64_t offset1 : 16;
    uint64_t offset2 : 32;
    uint64_t : 32;
} InterruptDescriptor;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    void *base;
} InterruptTableDescriptor;

// X86-64 Registers
typedef struct Registers {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
} Registers;

#define GATE_CALL 0xC
#define GATE_INTR 0xE
#define GATE_TRAP 0xF

void IntelSetInterrupt(int interrupt, void *handler, uint16_t type);

void IntelInitialize(uint64_t kernel_stack);

void *IntelGetCR3(void);
void IntelSetCR3(void *cr3);

static inline uint8_t IoIn8(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0"
                     : "=a"(ret)
                     : "Nd"(port));
    return ret;
}

static inline uint16_t IoIn16(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0"
                     : "=a"(ret)
                     : "Nd"(port));
    return ret;
}

static inline uint32_t IoIn32(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0"
                     : "=a"(ret)
                     : "Nd"(port));
    return ret;
}

static inline void IoOut8(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" ::"a"(val), "Nd"(port));

}

static inline void IoOut16(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" ::"a"(val), "Nd"(port));
}

static inline void IoOut32(uint16_t port, uint32_t val) {
    __asm__ volatile("outl %0, %1" ::"a"(val), "Nd"(port));
}

static inline void IoWait(void) {
    __asm__ volatile("outb %%al, $0x80"
                     :
                     : "a"(0));
}