/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: abi/nova_abi.h
 * Description: Frozen, Versioned Kernel ABI Specification (v1.0.0-LTS).
 * Enforces strict stability, capability-based authority, and backwards compatibility.
 * License: Apache-2.0
 */

#ifndef NOVA_ABI_H
#define NOVA_ABI_H

#include <stdint.h>
#include <stddef.h>

#define NOVA_ABI_VERSION_MAJOR 1
#define NOVA_ABI_VERSION_MINOR 0
#define NOVA_ABI_VERSION_PATCH 0
#define NOVA_ABI_SIGNATURE     0x4E4F56414F530100ULL /* 'NOVAOS\x01\x00' */

/* System Call Numbers */
#define SYS_NOVA_NULL          0
#define SYS_NOVA_LOG           1
#define SYS_NOVA_CAP_CREATE    2
#define SYS_NOVA_CAP_DELEGATE  3
#define SYS_NOVA_CAP_REVOKE    4
#define SYS_NOVA_IPC_CALL      5
#define SYS_NOVA_IPC_REPLY     6
#define SYS_NOVA_IPC_WAIT      7
#define SYS_NOVA_TASK_CREATE   8
#define SYS_NOVA_TASK_TERMINATE 9
#define SYS_NOVA_MEM_MAP       10
#define SYS_NOVA_MEM_UNMAP     11
#define SYS_NOVA_PORT_IO       12
#define SYS_NOVA_TIME_GET      13
#define SYS_NOVA_SYS_INFO      14
#define SYS_NOVA_MAX_SYSCALL   15

/* Capability Rights Bitmask */
#define CAP_RIGHT_READ         (1U << 0)
#define CAP_RIGHT_WRITE        (1U << 1)
#define CAP_RIGHT_EXECUTE      (1U << 2)
#define CAP_RIGHT_GRANT        (1U << 3)
#define CAP_RIGHT_REVOKE       (1U << 4)
#define CAP_RIGHT_IPC_SEND     (1U << 5)
#define CAP_RIGHT_IPC_RECV     (1U << 6)
#define CAP_RIGHT_MEM_MAP      (1U << 7)
#define CAP_RIGHT_PORT_IO      (1U << 8)
#define CAP_RIGHT_DEV_MGMT     (1U << 9)
#define CAP_RIGHT_SYSTEM_ADMIN (1U << 10)

/* Capability Object Types */
#define CAP_TYPE_NULL          0
#define CAP_TYPE_MEMORY        1
#define CAP_TYPE_IPC_ENDPOINT  2
#define CAP_TYPE_THREAD        3
#define CAP_TYPE_PROCESS       4
#define CAP_TYPE_DEVICE_PORT   5
#define CAP_TYPE_INTERRUPT     6

/* IPC Message Envelope (Exactly 64 bytes cache-line aligned) */
typedef struct __attribute__((aligned(64))) {
    uint32_t sender_pid;        /* 4 bytes */
    uint32_t target_endpoint;  /* 4 bytes */
    uint32_t flags;            /* 4 bytes */
    uint32_t message_type;     /* 4 bytes */
    uint64_t token;            /* 8 bytes */
    uint64_t transferred_cap;  /* 8 bytes */
    uint64_t sequence_id;      /* 8 bytes */
    uint64_t data[3];          /* 24 bytes inline fast payload */
} nova_ipc_msg_t;

/* System Information Structure */
typedef struct {
    uint64_t signature;
    uint32_t abi_version;
    uint32_t cpu_count;
    uint64_t total_memory_bytes;
    uint64_t free_memory_bytes;
    uint64_t uptime_ticks;
    uint64_t context_switches;
    uint64_t ipc_messages_transferred;
} nova_sys_info_t;

#endif /* NOVA_ABI_H */
