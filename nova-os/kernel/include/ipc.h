/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/ipc.h
 * Description: Fast Synchronous Rendezvous & Asynchronous Microkernel IPC.
 * License: Apache-2.0
 */

#ifndef NOVA_IPC_H
#define NOVA_IPC_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"
#include "nova_abi.h"

#define MAX_IPC_ENDPOINTS 128

typedef struct ipc_endpoint {
    uint32_t id;
    uint32_t owner_task_id;
    bool     is_open;
    nova_ipc_msg_t queued_msg;
    bool     has_message;
    uint32_t waiting_sender_task_id;
    uint32_t waiting_receiver_task_id;
} ipc_endpoint_t;

void ipc_init(void);
int32_t ipc_create_endpoint(uint32_t owner_task_id);
status_t ipc_send(uint32_t endpoint_id, const nova_ipc_msg_t *msg);
status_t ipc_recv(uint32_t endpoint_id, nova_ipc_msg_t *msg_out);
status_t ipc_call(uint32_t endpoint_id, const nova_ipc_msg_t *req, nova_ipc_msg_t *resp);
void ipc_close_endpoint(uint32_t endpoint_id);

#endif /* NOVA_IPC_H */
