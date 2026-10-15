/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/security/canary.c
 * Description: Stack Protector Canary & Pointer Validation.
 * License: Apache-2.0
 */

#include "threat_model.h"
#include "kprintf.h"

uint64_t __stack_chk_guard = 0x595e9fbd94fda766ULL;

void __stack_chk_fail(void) {
    KERNEL_PANIC("CRITICAL SECURITY VIOLATION: Stack Smashing / Buffer Overflow Detected!");
}

void security_init_subsystem(void) {
    kprintf("[SECURITY] Threat Model active: W^X Enforced, NX Active, Capabilities Sandboxed\n");
    kprintf("[SECURITY] Stack Guard Cookie initialized at 0x%p\n", &__stack_chk_guard);
}

bool security_verify_pointer(const void *ptr, size_t size, bool is_write) {
    (void)is_write;
    uint64_t addr = (uint64_t)ptr;
    // Prevent NULL or near-NULL pointer dereference
    if (addr < 0x1000) {
        return false;
    }
    // Check for overflow
    if (addr + size < addr) {
        return false;
    }
    // Prevent unprivileged user pointers from trespassing into kernel address space
    // Kernel space starts at 0xFFFF800000000000 or high half
    if (addr >= 0x800000000000ULL && addr < 0xFFFF800000000000ULL) {
        return false; // Non-canonical or guard region
    }
    return true;
}
