#pragma once

#include <stdint.h>
#include <mem/vmm.h>

typedef void (*TaskEntry)(void);

#define TASK_STATE_READY   0
#define TASK_STATE_BLOCKED 1
#define TASK_STATE_PAUSED  2
#define TASK_STATE_STOPPED 3

typedef struct Task {
    uint64_t pid;
    uint32_t state;

    const char *name;

    TaskEntry entry;

    PageDirectory *memory;

    uint64_t rip;
    uint64_t rsp, rbp;

    struct Task *next;
} Task;

void TskInitialize(void);
void TskPrintTasks(void);

Task *TskCreateTask(const char *name, TaskEntry entry);

Task *TskCreateKernelTask(const char *name, TaskEntry entry);
Task *TskCreateUserTask(const char *name, TaskEntry entry);

void TskSchedule(void);
