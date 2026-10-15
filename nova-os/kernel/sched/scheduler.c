/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/sched/scheduler.c
 * Description: Preemptive Multi-Level Priority Scheduler with Round-Robin.
 * License: Apache-2.0
 */

#include "sched.h"
#include "kprintf.h"
#include "gdt.h"
#include "cpu.h"

static task_t *current_task = NULL;
static task_t *task_queue[PRIORITY_LEVELS] = { NULL };
static uint64_t total_context_switches = 0;

static void idle_task_loop(void) {
    while (1) {
        hlt();
    }
}

void sched_init(void) {
    // Create idle task at lowest priority
    task_t *idle = task_create("idle", idle_task_loop, 3, false);
    task_queue[3] = idle;
    current_task = idle;
    current_task->state = TASK_STATE_RUNNING;

    kprintf("[SCHED] Preemptive scheduler initialized with %u priority levels\n", PRIORITY_LEVELS);
}

void sched_add_task(task_t *t) {
    if (!t) return;
    t->state = TASK_STATE_READY;
    uint32_t p = t->priority < PRIORITY_LEVELS ? t->priority : 3;

    t->next = task_queue[p];
    task_queue[p] = t;
}

task_t *sched_get_current(void) {
    return current_task;
}

void sched_yield(void) {
    task_t *prev = current_task;
    task_t *next = NULL;

    // Pick highest priority runnable task
    for (int p = 0; p < PRIORITY_LEVELS; p++) {
        task_t *t = task_queue[p];
        while (t) {
            if (t->state == TASK_STATE_READY && t != prev) {
                next = t;
                break;
            }
            t = t->next;
        }
        if (next) break;
    }

    if (!next) {
        if (prev && prev->state == TASK_STATE_RUNNING) {
            return; // Continue running current task
        }
        next = task_queue[3]; // Fall back to idle
    }

    if (next && next != prev) {
        total_context_switches++;
        if (prev && prev->state == TASK_STATE_RUNNING) {
            prev->state = TASK_STATE_READY;
        }

        next->state = TASK_STATE_RUNNING;
        current_task = next;

        // Switch TSS kernel stack for Ring 3 privilege transitions
        tss_set_stack(next->kernel_stack_top);

        // Switch address space if different
        if (next->cr3 && next->cr3 != prev->cr3) {
            vmm_switch_address_space(next->cr3);
        }

        context_switch(&prev->context, &next->context);
    }
}

void sched_tick(void) {
    if (!current_task) return;
    current_task->cpu_time_ticks++;

    if (current_task->quantum_remaining > 0) {
        current_task->quantum_remaining--;
    }

    if (current_task->quantum_remaining == 0) {
        current_task->quantum_remaining = (4 - current_task->priority) * 10;
        sched_yield();
    }
}

uint64_t sched_get_context_switches(void) {
    return total_context_switches;
}
