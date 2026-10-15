/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/sched.h
 * Description: Preemptive Priority Scheduler and Process/Thread Control.
 * License: Apache-2.0
 */

#ifndef NOVA_SCHED_H
#define NOVA_SCHED_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"
#include "vmm.h"
#include "capabilities.h"

#define MAX_TASKS 64
#define PRIORITY_LEVELS 4
#define KERNEL_STACK_SIZE 16384

typedef enum {
    TASK_STATE_EMBRYO,
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED_IPC,
    TASK_STATE_SLEEPING,
    TASK_STATE_DEAD
} task_state_t;

/* Architecture-specific CPU context saved during switch */
typedef struct {
    uint64_t rsp;
    uint64_t r15, r14, r13, r12, rbx, rbp;
    uint64_t rflags;
} cpu_context_t;

typedef struct task {
    uint32_t           id;
    char               name[32];
    task_state_t       state;
    uint32_t           priority; /* 0 = highest, 3 = idle */
    pml4_t            *cr3;
    uint64_t           kernel_stack_top;
    uint64_t           user_stack_top;
    cpu_context_t      context;
    capability_table_t cap_table;
    uint64_t           cpu_time_ticks;
    uint64_t           quantum_remaining;
    struct task       *next;
} task_t;

void sched_init(void);
task_t *task_create(const char *name, void (*entry_point)(void), uint32_t priority, bool is_user);
void task_exit(int code);
void sched_yield(void);
void sched_tick(void);
task_t *sched_get_current(void);
void context_switch(cpu_context_t *old_ctx, cpu_context_t *new_ctx);

#endif /* NOVA_SCHED_H */
