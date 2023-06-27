#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "limine.h"
#include <cpu/acpi.h>
#include <cpu/apic.h>
#include <cpu/intel.h>

#include <mem/heap.h>
#include <mem/pmm.h>
#include <mem/vmm.h>

#include <tsk/sched.h>

#include <utl/serial.h>

static volatile struct limine_framebuffer_request framebuffer_request = {
        .id = LIMINE_FRAMEBUFFER_REQUEST,
        .revision = 1,
};

static volatile struct limine_smp_request smp_request = {
        .id = LIMINE_SMP_REQUEST,
        .revision = 0,
};

static void TestTask1() {
    while (1) {
        ComPrint("[TSK] Test task 1\n");
    }
}

static void TestTask2() {
    while (1) {
        ComPrint("[TSK] Test task 2\n");
    }
}

void TimerHandler(uint8_t irq, void *data) {
    ComPrint("[INTR] Timer!!!!!!!\n");
}

void Ps2KeyboardHandler(uint8_t irq, void *data) {
    uint8_t keyboard = IoIn8(0x60);
    ComPrint("Keyboard: %x\n", keyboard);
}

uint8_t mouse_cycle = 0;//unsigned char
int8_t mouse_byte[3];   //signed char
int8_t mouse_x = 0;     //signed char
int8_t mouse_y = 0;     //signed char

void Ps2MouseHandler(uint8_t irq, void *data) {
    switch (mouse_cycle) {
        case 0:
            mouse_byte[0] = IoIn8(0x60);
            mouse_cycle++;
            break;
        case 1:
            mouse_byte[1] = IoIn8(0x60);
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2] = IoIn8(0x60);
            mouse_x = mouse_byte[1];
            mouse_y = mouse_byte[2];
            mouse_cycle = 0;
            break;
    }

    ComPrint("Mouse X: %d, Y: %d   Buttons: %d %d %d\n", mouse_x, mouse_y, mouse_byte[0] & 0x1, (mouse_byte[0] & 0x2) >> 1, (mouse_byte[0] & 0x4) >> 2);
}

static void mouse_wait(uint8_t a_type)
{
    uint32_t _time_out = 100000;//unsigned int
    if (a_type == 0) {
        while (_time_out--)//Data
        {
            if ((IoIn8(0x64) & 1) == 1) {
                return;
            }
        }
        return;
    } else {
        while (_time_out--)//Signal
        {
            if ((IoIn8(0x64) & 2) == 0) {
                return;
            }
        }
        return;
    }
}

static void mouse_write(uint8_t a_write) {
    mouse_wait(1);
    IoOut8(0x64, 0xD4);
    mouse_wait(1);
    IoOut8(0x60, a_write);
}

uint8_t mouse_read() {
    mouse_wait(0);
    return IoIn8(0x60);
}

typedef struct {
    union {
        uint32_t ax;
        uint32_t magic;
    };
    union {
        uint32_t bx;
        size_t size;
    };
    union {
        uint32_t cx;
        uint16_t command;
    };
    union {
        uint32_t dx;
        uint16_t port;
    };
    uint32_t si;
    uint32_t di;
} vmware_cmd;

#define VMWARE_MAGIC  0x564D5868
#define VMWARE_PORT   0x5658
#define VMWARE_PORTHB 0x5659

void vmware_send(vmware_cmd * cmd) {
    cmd->magic = VMWARE_MAGIC;
    cmd->port = VMWARE_PORT;
    __asm__ volatile("in %%dx, %0" : "+a"(cmd->ax), "+b"(cmd->bx), "+c"(cmd->cx), "+d"(cmd->dx), "+S"(cmd->si), "+D"(cmd->di));
}

static void vmware_send_hb(vmware_cmd * cmd) {
    cmd->magic = VMWARE_MAGIC;
    cmd->port = VMWARE_PORTHB;
    __asm__ volatile("cld; rep; outsb" : "+a"(cmd->ax), "+b"(cmd->bx), "+c"(cmd->cx), "+d"(cmd->dx), "+S"(cmd->si), "+D"(cmd->di));
}

static void vmware_get_hb(vmware_cmd * cmd) {
    cmd->magic = VMWARE_MAGIC;
    cmd->port = VMWARE_PORTHB;
    __asm__ volatile("cld; rep; insb" : "+a"(cmd->ax), "+b"(cmd->bx), "+c"(cmd->cx), "+d"(cmd->dx), "+S"(cmd->si), "+D"(cmd->di));
}

int is_vmware_backdoor(void) {
    vmware_cmd cmd;
    cmd.bx = ~VMWARE_MAGIC;
    cmd.command = 10;
    vmware_send(&cmd);

    if (cmd.bx != VMWARE_MAGIC || cmd.ax == 0xFFFFFFFF) {
        /* Not a backdoor! */
        return 0;
    }

    return 1;
}

#if 0
// Entry-point for secondary cores
static void KeSMPMain(struct limine_smp_info *info) {
    ComPrint("Secondary core %d started\n", info->processor_id);

    while (1) {
        // Wait for an interrupt or a task to be scheduled
        __asm__ volatile("hlt");
    }
}
#endif

// Entry-point for primary core
void KeMain(void) {
    uint64_t stack = 0;
    __asm__ volatile("mov %%rsp, %0"
                     : "=r"(stack)::"memory");

    ComPrint("[KERNEL] Primary core started.\n");

    IntelInitialize(stack);

    MmInitialize();
    MmInitializePaging();
    MmInitializeHeap();

    TskInitialize();

    AcpiInitialize();

    if (is_vmware_backdoor()) {
        ComPrint("[KERNEL] VMware backdoor detected.\n");
    }

    // Set up the PS/2 keyboard as a test
    ApicRegisterIrqHandler(1, Ps2KeyboardHandler, 0);
    ApicEnableInterrupt(1);

    uint8_t _status;//unsigned char

    //Enable the auxiliary mouse device
    mouse_wait(1);
    IoOut8(0x64, 0xA8);

    //Enable the interrupts
    mouse_wait(1);
    IoOut8(0x64, 0x20);
    mouse_wait(0);
    _status = (IoIn8(0x60) | 2);
    mouse_wait(1);
    IoOut8(0x64, 0x60);
    mouse_wait(1);
    IoOut8(0x60, _status);

    mouse_write(0xF6);
    mouse_read();
    mouse_write(0xF4);
    mouse_read();


    ApicRegisterIrqHandler(12, Ps2MouseHandler, 0);
    ApicEnableInterrupt(12);


    TskCreateKernelTask("Test Task 1", TestTask1);
    TskCreateKernelTask("Test Task 2", TestTask2);

    TskPrintTasks();
#if 0
    // Start up the other cores
    struct limine_smp_response *smp = smp_request.response;
    for (size_t i = 0; i < smp->cpu_count; i++)
        smp->cpus[i]->goto_address = KeSMPMain;
#endif

    // Main loop
    while (1) __asm__ volatile("hlt");
}
