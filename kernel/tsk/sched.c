#include "sched.h"

#include <cpu/intel.h>
#include <mem/heap.h>
#include <utl/serial.h>

Task *kTasks = 0;
Task *kLastTask = 0;

Task *kTask = 0;

uint64_t kNextPid = 0;


static void TskIdleTask() {
    while (1)
        __asm__ __volatile__("hlt");
}

void TskInitialize(void) {
    TskCreateKernelTask("Kernel Idle", TskIdleTask);
}

void TskPrintTasks(void) {
    ComPrint("[TSK] Task list:\n");
    for (Task *task = kTasks; task; task = task->next) {
        ComPrint("[TSK]    Task(%d): %s", task->pid, task->name);
        if (task == kTask)
            ComPrint(" (current)");
        ComPrint("\n");
    }
}

Task *TskCreateTask(const char *name, TaskEntry entry) {
    Task *task = (Task *) kmalloc(sizeof(Task));
    RtZeroMemory(task, sizeof(Task));

    if (!kTasks) {
        kTasks = task;
        kLastTask = task;
    } else {
        kLastTask->next = task;
        kLastTask = task;
    }

    task->pid = kNextPid++;
    task->name = name;
    task->entry = entry;

    return task;
}

Task *TskCreateKernelTask(const char *name, TaskEntry entry) {
    Task *task = TskCreateTask(name, entry);
    task->memory = IntelGetCR3();
    return task;
}

void TskSchedule(void) {
    if (!kTask) {
        kTask = kTasks;
        return;
    }

    Task *next = kTask->next;
    if (!next)
        next = kTasks;

    kTask = next;

    IntelSetCR3(kTask->memory);


}
