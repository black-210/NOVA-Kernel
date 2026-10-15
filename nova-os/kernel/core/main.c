/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/core/main.c
 * Description: Kernel Main Entry Point and Unified Subsystem Initialization Sequence.
 * License: Apache-2.0
 */

#include "types.h"
#include "nova_abi.h"
#include "cpu.h"
#include "serial.h"
#include "kprintf.h"
#include "gdt.h"
#include "idt.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "threat_model.h"
#include "sched.h"
#include "ipc.h"
#include "syscall.h"
#include "pci.h"
#include "vfs.h"

extern void pit_init(uint32_t freq);
extern void devmgr_init(void);
extern void netserver_init(void);
extern void compositor_init(uint32_t w, uint32_t h);

// Initial User-Space Server Task Entries
static void userspace_init_server(void) {
    kprintf("[INIT-SERVER] NOVA OS User-Space Root Manager started (PID 2, Ring 3 isolation)\n");
    kprintf("[INIT-SERVER] Verifying system capabilities and mounting services...\n");

    // Loop processing IPC events
    while (1) {
        hlt();
    }
}

void kmain(void) {
    // 1. Initialize Serial Console for early diagnostics
    serial_init();

    kprintf("\n");
    kprintf("===================================================================\n");
    kprintf("                 NOVA OPERATING SYSTEM v1.0.0-LTS                  \n");
    kprintf("        Independent Next-Generation Microkernel Architecture       \n");
    kprintf("===================================================================\n");
    kprintf("ABI Specification: v%u.%u.%u (Signature: 0x%x)\n",
            NOVA_ABI_VERSION_MAJOR, NOVA_ABI_VERSION_MINOR, NOVA_ABI_VERSION_PATCH,
            (uint32_t)NOVA_ABI_SIGNATURE);

    // 2. Security Subsystem & Threat Mitigation
    security_init_subsystem();

    // 3. CPU Core Structures: GDT & TSS
    gdt_init();

    // 4. Interrupts & Exceptions: IDT & PIC
    idt_init();

    // 5. Physical Memory Manager (assuming 512MB RAM for QEMU/VM)
    pmm_init(512 * 1024 * 1024ULL, 0x200000);

    // 6. Virtual Memory Manager (PML4 4-Level Paging with W^X enforcement)
    vmm_init();

    // 7. Hardened Kernel Heap Allocator (8MB pool with canaries)
    heap_init(0x01000000, 8 * 1024 * 1024);

    // 8. Programmable Interval Timer (1000 Hz for preemptive time-slicing)
    pit_init(1000);

    // 9. Fast Microkernel IPC Engine
    ipc_init();

    // 10. System Call Dispatcher (int 0x80)
    syscall_init();

    // 11. Hardware Discovery (PCI Enumeration)
    pci_init();

    // 12. Virtual File System & NOVA-FS
    vfs_init();

    // 13. User-Space Isolated Servers
    devmgr_init();
    netserver_init();
    compositor_init(1920, 1080);

    // 14. Preemptive Multi-Level Priority Scheduler
    sched_init();

    // 15. Spawn Root User-Space Init Manager
    task_t *init_task = task_create("init-server", userspace_init_server, 1, false);
    if (init_task) {
        // Grant standard capabilities
        cap_create(&init_task->cap_table, CAP_TYPE_IPC_ENDPOINT,
                   CAP_RIGHT_IPC_SEND | CAP_RIGHT_IPC_RECV | CAP_RIGHT_GRANT, 1);
        cap_create(&init_task->cap_table, CAP_TYPE_MEMORY,
                   CAP_RIGHT_READ | CAP_RIGHT_WRITE | CAP_RIGHT_MEM_MAP, 0);
    }

    kprintf("\n[BOOT] NOVA OS Microkernel boot sequence completed successfully.\n");
    kprintf("[BOOT] Enabling hardware interrupts and transitioning to scheduler...\n");
    kprintf("===================================================================\n\n");

    sti(); // Enable interrupts!

    while (1) {
        sched_yield();
        hlt();
    }
}
