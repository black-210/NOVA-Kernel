/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/capabilities.h
 * Description: Unforgeable Capability Access Control Architecture.
 * Eliminates ambient authority and replaces Linux coarse root privileges with fine-grained tokens.
 * License: Apache-2.0
 */

#ifndef NOVA_CAPABILITIES_H
#define NOVA_CAPABILITIES_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"
#include "nova_abi.h"

#define MAX_CAPABILITIES_PER_PROCESS 256

typedef struct {
    uint32_t type;         /* CAP_TYPE_* */
    uint32_t rights;       /* CAP_RIGHT_* */
    uint64_t object_id;    /* Pointer or ID to target kernel object */
    uint64_t badge;        /* Process isolation badge */
    bool     valid;
} capability_t;

typedef struct {
    capability_t entries[MAX_CAPABILITIES_PER_PROCESS];
    uint32_t count;
} capability_table_t;

void cap_table_init(capability_table_t *table);
int32_t cap_create(capability_table_t *table, uint32_t type, uint32_t rights, uint64_t object_id);
bool cap_validate(capability_table_t *table, uint32_t slot, uint32_t required_type, uint32_t required_rights);
int32_t cap_delegate(capability_table_t *src_table, uint32_t src_slot,
                     capability_table_t *dst_table, uint32_t rights_mask);
bool cap_revoke(capability_table_t *table, uint32_t slot);

#endif /* NOVA_CAPABILITIES_H */
