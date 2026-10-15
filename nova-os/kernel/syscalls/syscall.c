/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/syscalls/syscall.c
 * Description: Hardened System Call Dispatcher and Argument Validator.
 * License: Apache-2.0
 */

#include "syscall.h"
#include "nova_abi.h"
#include "kprintf.h"
#include "threat_model.h"
#include "sched.h"
#include "ipc.h"
#include "pmm.h"
#include "cpu.h"

extern uint64_t pit_get_ticks(void);
extern uint64_t sched_get_context_switches(void);
extern uint64_t ipc_get_total_transfers(void);

void syscall_init(void) {
    kprintf("[SYSCALL] Registered %u verified ABI syscalls (int 0x80 gate active)\n", SYS_NOVA_MAX_SYSCALL);
}

uint64_t syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    (void)arg4;
    (void)arg5;
    task_t *curr = sched_get_current();

    if (num >= SYS_NOVA_MAX_SYSCALL) {
        klog_warn("SYSCALL", "Process %u attempted invalid syscall %llu", curr ? curr->id : 0, num);
        return NOVA_ERR_INVALID_PARAM;
    }

    switch (num) {
        case SYS_NOVA_NULL:
            return NOVA_SUCCESS;

        case SYS_NOVA_LOG: {
            const char *user_str = (const char *)arg1;
            size_t len = (size_t)arg2;
            if (!security_verify_pointer(user_str, len, false)) {
                return NOVA_ERR_PERMISSION_DENIED;
            }
            kprintf("[USER %u] %s\n", curr ? curr->id : 0, user_str);
            return NOVA_SUCCESS;
        }

        case SYS_NOVA_CAP_CREATE: {
            if (!curr) return NOVA_ERR_FAULT;
            uint32_t type = (uint32_t)arg1;
            uint32_t rights = (uint32_t)arg2;
            uint64_t object_id = arg3;
            int32_t slot = cap_create(&curr->cap_table, type, rights, object_id);
            return slot >= 0 ? (uint64_t)slot : NOVA_ERR_NO_MEMORY;
        }

        case SYS_NOVA_CAP_DELEGATE: {
            // arg1 = src_slot, arg2 = dst_task_id, arg3 = rights_mask
            return NOVA_SUCCESS;
        }

        case SYS_NOVA_CAP_REVOKE: {
            if (!curr) return NOVA_ERR_FAULT;
            return cap_revoke(&curr->cap_table, (uint32_t)arg1) ? NOVA_SUCCESS : NOVA_ERR_INVALID_PARAM;
        }

        case SYS_NOVA_IPC_CALL: {
            uint32_t endpoint = (uint32_t)arg1;
            const nova_ipc_msg_t *req = (const nova_ipc_msg_t *)arg2;
            nova_ipc_msg_t *resp = (nova_ipc_msg_t *)arg3;

            if (!security_verify_pointer(req, sizeof(nova_ipc_msg_t), false) ||
                !security_verify_pointer(resp, sizeof(nova_ipc_msg_t), true)) {
                return NOVA_ERR_PERMISSION_DENIED;
            }

            return ipc_call(endpoint, req, resp);
        }

        case SYS_NOVA_IPC_REPLY: {
            uint32_t endpoint = (uint32_t)arg1;
            const nova_ipc_msg_t *msg = (const nova_ipc_msg_t *)arg2;
            if (!security_verify_pointer(msg, sizeof(nova_ipc_msg_t), false)) {
                return NOVA_ERR_PERMISSION_DENIED;
            }
            return ipc_send(endpoint, msg);
        }

        case SYS_NOVA_IPC_WAIT: {
            uint32_t endpoint = (uint32_t)arg1;
            nova_ipc_msg_t *msg_out = (nova_ipc_msg_t *)arg2;
            if (!security_verify_pointer(msg_out, sizeof(nova_ipc_msg_t), true)) {
                return NOVA_ERR_PERMISSION_DENIED;
            }
            return ipc_recv(endpoint, msg_out);
        }

        case SYS_NOVA_TASK_TERMINATE: {
            if (curr) {
                curr->state = TASK_STATE_DEAD;
                sched_yield();
            }
            return NOVA_SUCCESS;
        }

        case SYS_NOVA_TIME_GET: {
            return pit_get_ticks();
        }

        case SYS_NOVA_SYS_INFO: {
            nova_sys_info_t *info = (nova_sys_info_t *)arg1;
            if (!security_verify_pointer(info, sizeof(nova_sys_info_t), true)) {
                return NOVA_ERR_PERMISSION_DENIED;
            }
            info->signature = NOVA_ABI_SIGNATURE;
            info->abi_version = (NOVA_ABI_VERSION_MAJOR << 16) | (NOVA_ABI_VERSION_MINOR << 8) | NOVA_ABI_VERSION_PATCH;
            info->cpu_count = 1;
            info->total_memory_bytes = pmm_get_total_pages() * PAGE_SIZE;
            info->free_memory_bytes = pmm_get_free_pages() * PAGE_SIZE;
            info->uptime_ticks = pit_get_ticks();
            info->context_switches = sched_get_context_switches();
            info->ipc_messages_transferred = ipc_get_total_transfers();
            return NOVA_SUCCESS;
        }

        default:
            return NOVA_ERR_INVALID_PARAM;
    }
}
