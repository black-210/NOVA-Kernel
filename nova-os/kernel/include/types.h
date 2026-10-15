/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/types.h
 * Description: Fundamental architecture-independent and x86_64 types.
 * License: Apache-2.0
 */

#ifndef NOVA_TYPES_H
#define NOVA_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;
typedef uint32_t pid_t;
typedef uint32_t tid_t;
typedef uint32_t cap_handle_t;
typedef uint32_t status_t;

/* Status codes */
#define NOVA_SUCCESS              0x00000000
#define NOVA_ERR_INVALID_PARAM    0x80000001
#define NOVA_ERR_NO_MEMORY        0x80000002
#define NOVA_ERR_PERMISSION_DENIED 0x80000003
#define NOVA_ERR_NOT_FOUND        0x80000004
#define NOVA_ERR_BUFFER_TOO_SMALL 0x80000005
#define NOVA_ERR_BUSY             0x80000006
#define NOVA_ERR_TIMEOUT          0x80000007
#define NOVA_ERR_CAPABILITY_REVOKED 0x80000008
#define NOVA_ERR_IPC_CLOSED       0x80000009
#define NOVA_ERR_FAULT            0x8000000A

#define PAGE_SIZE 4096ULL
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define ALIGN_UP(addr, align)   (((addr) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))

#endif /* NOVA_TYPES_H */
