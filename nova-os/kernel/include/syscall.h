/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/syscall.h
 * Description: System Call Dispatcher and Verification Interface.
 * License: Apache-2.0
 */

#ifndef NOVA_SYSCALL_H
#define NOVA_SYSCALL_H

#include <stdint.h>
#include "types.h"
#include "idt.h"

void syscall_init(void);
uint64_t syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

#endif /* NOVA_SYSCALL_H */
