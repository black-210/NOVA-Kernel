/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/sched/process.c
 * Description: Task Creation, Process Control Block, and Stack Initialization.
 * License: Apache-2.0
 */

#include "sched.h"
#include "heap.h"
#include "pmm.h"
#include "kprintf.h"
#include "gdt.h"

static uint32_t next_task_id = 1;

task_t *task_create(const char *name, void (*entry_point)(void), uint32_t priority, bool is_user) {
    task_t *t = (task_t *)kzalloc(sizeof(task_t));
    if (!t) return NULL;

    t->id = next_task_id++;
    kstrncpy(t->name, name ? name : "task", sizeof(t->name));
    t->state = TASK_STATE_READY;
    t->priority = priority < PRIORITY_LEVELS ? priority : 3;
    t->quantum_remaining = (4 - t->priority) * 10; // High priority gets larger quantum
    t->cpu_time_ticks = 0;

    // Allocate kernel stack (16KB)
    void *kstack = kmalloc(KERNEL_STACK_SIZE);
    if (!kstack) {
        kfree(t);
        return NULL;
    }
    t->kernel_stack_top = (uint64_t)kstack + KERNEL_STACK_SIZE;

    // Address space
    if (is_user) {
        t->cr3 = vmm_create_address_space();
        // Allocate 16KB user stack
        void *ustack_phys = pmm_alloc_pages(4);
        uint64_t ustack_virt = 0x7FFFFFFF0000ULL;
        for (int i = 0; i < 4; i++) {
            vmm_map_page(t->cr3, ustack_virt + (i * 4096),
                         (uint64_t)ustack_phys + (i * 4096), PAGE_USER_RW);
        }
        t->user_stack_top = ustack_virt + (4 * 4096) - 16;
    } else {
        t->cr3 = vmm_get_kernel_pml4();
        t->user_stack_top = 0;
    }

    // Initialize capability table
    cap_table_init(&t->cap_table);

    // Initial CPU context setup
    uint64_t *sp = (uint64_t *)t->kernel_stack_top;
    *(--sp) = (uint64_t)entry_point; // Return address for context_switch
    *(--sp) = 0x202;                // RFLAGS with IF enabled
    *(--sp) = 0;                    // RBP
    *(--sp) = 0;                    // RBX
    *(--sp) = 0;                    // R12
    *(--sp) = 0;                    // R13
    *(--sp) = 0;                    // R14
    *(--sp) = 0;                    // R15

    t->context.rsp = (uint64_t)sp;
    t->context.rflags = 0x202;

    kprintf("[TASK] Created task #%u '%s' (prio=%u, is_user=%s, kstack=0x%p)\n",
            t->id, t->name, t->priority, is_user ? "true" : "false", (void *)t->kernel_stack_top);

    return t;
}
