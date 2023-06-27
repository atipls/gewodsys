#pragma once

#include <stdint.h>

#include <cpu/apic.h>
#include <dev/pci.h>

typedef volatile struct __attribute__((packed)) {
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

typedef volatile struct __attribute__((packed)) {
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

enum {
    kUsbCmdRun = 1 << 0,
    kUsbCmdHcReset = 1 << 1,
    kUsbCmdInterruptEnable = 1 << 2,
    kUsbCmdErrorEnable = 1 << 3,

    kUsbStsHcHalted = 1 << 0,
    kUsbStsHostSystemError = 1 << 2,
    kUsbStsEventInterrupt = 1 << 3,
    kUsbStsPortChangeDetect = 1 << 4,
    kUsbStsControllerNotReady = 1 << 11,
    kUsbStsHcError = 1 << 12,
};

typedef volatile struct {
    union {
        uint32_t iman_r;
        struct {
            uint32_t ip : 1;
            uint32_t ie : 1;
            uint32_t : 30;
        } iman;
    };
    union {
        uint32_t imod_r;
        struct {
            uint32_t imodi : 16;
            uint32_t imodc : 16;
        } imod;
    };
    uint32_t erstsz : 16;
    uint32_t : 16;
    union {
        uint64_t erstba_r;
        struct {
            uint32_t erstba_lo;
            uint32_t erstba_hi;
        } erstba;
    };
    union {
        uint64_t erdp_r;
        struct {
            uint32_t desi : 3;
            uint32_t busy : 1;
            uint32_t erdp_lo : 28;
            uint32_t erdp_hi;
        } erdp;
    };
} XhciInterrupterRegisters;

enum {
    kImanInterruptPending = 1 << 0,
};

typedef volatile struct __attribute__((packed)) {
    uint32_t mfindex : 16;
    uint32_t : 16;
    uint32_t reserved[7];
    XhciInterrupterRegisters interrupters[1024];
} XhciRuntimeRegisters;

typedef volatile union {
    uint32_t db;
    struct {
        uint32_t target : 8;
        uint32_t : 8;
        uint32_t task_id : 16;
    };
} XhciDoorbellRegister;

enum {
    kTrbNormal = 1,
    kTrbSetupStage = 2,
    kTrbDataStage = 3,
    kTrbStatusStage = 4,
    kTrbIsoch = 5,
    kTrbLink = 6,
    kTrbEventData = 7,
    kTrbNoop = 8,
    kTrbEnableSlotCmd = 9,
    kTrbDisableSlotCmd = 10,
    kTrbAddressDeviceCmd = 11,
    kTrbConfigureEndpointCmd = 12,
    kTrbEvaluateContextCmd = 13,
    kTrbResetEndpointCmd = 14,
    kTrbStopEndpointCmd = 15,
    kTrbSetDqPtrCmd = 16,
    kTrbResetDeviceCmd = 17,
    kTrbForceHeaderCmd = 22,
    kTrbNoopCmd = 23,
    kTrbTransferEvent = 32,
    kTrbCommandCompleteEvent = 33,
    kTrbPortStatusEvent = 34,
    kTrbHostControllerEvent = 37,
    kTrbDeviceNotificationEvent = 38,
    kTrbMfindexEvent = 39,
};

typedef volatile struct {
    uint32_t : 32;        // reserved
    uint32_t : 32;        // reserved
    uint32_t : 32;        // reserved
    uint32_t cycle : 1;   // cycle bit
    uint32_t : 9;         // reserved
    uint32_t trb_type : 6;// trb type
    uint32_t : 16;        // reserved
} XhciTrb;

typedef struct {
    uint64_t rs_addr;         // ring segment base address
    uint32_t : 24;            // reserved
    uint32_t target : 8;      // interrupter target
    uint32_t cycle : 1;       // cycle bit
    uint32_t toggle_cycle : 1;// evaluate next trb
    uint32_t : 2;             // reserved
    uint32_t ch : 1;          // chain bit
    uint32_t ioc : 1;         // interrupt on completion
    uint32_t : 4;             // reserved
    uint32_t trb_type : 6;    // trb type
    uint32_t : 16;            // reserved
} XhciTrbLink;

typedef volatile struct {
    XhciTrb *ptr;
    uint32_t index;
    uint32_t max_index;
    int cycle;
} XhciRing;


typedef struct {
    uint64_t rs_addr;
    uint32_t rs_size;
    uint32_t : 32;
} XhciErstEntry;

typedef struct {
    uint8_t index;
    uint8_t vector;
    XhciErstEntry *erst;
    XhciRing *ring;
} XhciInterrupter;


#define CMD_RING_SIZE 256
#define EVT_RING_SIZE 256
#define XFER_RING_SIZE 256
#define ERST_SIZE 1

#define INTERRUPT_BM_GET(dev, i) (dev->interrupt_bitmap[i / 8] & (1 << (i % 8)))
#define INTERRUPT_BM_SET(dev, i) (dev->interrupt_bitmap[i / 8] |= (1 << (i % 8)))
#define INTERRUPT_BM_CLR(dev, i) (dev->interrupt_bitmap[i / 8] &= ~(1 << (i % 8)))

typedef struct {
    PciDevice *pci;
    void *volatile mmio;
    XhciCapabilityRegisters *volatile cap;
    XhciOperationalRegisters *volatile op;
    XhciRuntimeRegisters *volatile rt;
    XhciDoorbellRegister *volatile db;
    void *volatile xcap;
    uint64_t dcbaap;

    uint64_t interrupter_bitmap_size;
    uint8_t *interrupter_bitmap;

    XhciInterrupter *interrupter;

    XhciRing *cmd;
    XhciRing *evt;
} XhciDevice;

#define INTERRUPTER_BITMAP_GET(dev, i) (dev->interrupter_bitmap[i / 8] & (1 << (i % 8)))
#define INTERRUPTER_BITMAP_SET(dev, i) (dev->interrupter_bitmap[i / 8] |= (1 << (i % 8)))

XhciRing *XhciRingCreate(uint64_t size);
void XhciRingDestroy(XhciRing *ring);

int XhciRingAdd(XhciRing *ring, XhciTrb *trb);
int XhciRingGet(XhciRing *ring, XhciTrb *trb);

uint64_t XhciRingGetPhysicalAddress(XhciRing *ring);
uint64_t XhciRingSize(XhciRing *ring);

XhciInterrupter *XhciInterrupterCreate(XhciDevice *xhci, IrqHandlerFn handler, void *data);

extern PciDriver kXhciDriver;