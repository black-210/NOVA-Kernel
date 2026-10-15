/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/security/capabilities.c
 * Description: Unforgeable Capability Token & Access Authority Engine.
 * Replaces Linux ambient root privileges with non-delegable/restricted capabilities.
 * License: Apache-2.0
 */

#include "capabilities.h"
#include "kprintf.h"

void cap_table_init(capability_table_t *table) {
    if (!table) return;
    for (int i = 0; i < MAX_CAPABILITIES_PER_PROCESS; i++) {
        table->entries[i].valid = false;
        table->entries[i].type = CAP_TYPE_NULL;
        table->entries[i].rights = 0;
        table->entries[i].object_id = 0;
        table->entries[i].badge = 0;
    }
    table->count = 0;
}

int32_t cap_create(capability_table_t *table, uint32_t type, uint32_t rights, uint64_t object_id) {
    if (!table) return -1;
    for (int i = 0; i < MAX_CAPABILITIES_PER_PROCESS; i++) {
        if (!table->entries[i].valid) {
            table->entries[i].valid = true;
            table->entries[i].type = type;
            table->entries[i].rights = rights;
            table->entries[i].object_id = object_id;
            table->entries[i].badge = (uint64_t)i ^ 0x55AA55AAULL;
            table->count++;
            return i;
        }
    }
    klog_warn("CAPABILITY", "Capability table full, cannot allocate new token");
    return -1;
}

bool cap_validate(capability_table_t *table, uint32_t slot, uint32_t required_type, uint32_t required_rights) {
    if (!table || slot >= MAX_CAPABILITIES_PER_PROCESS) return false;
    capability_t *entry = &table->entries[slot];
    if (!entry->valid) return false;

    if (entry->type != required_type) {
        klog_sec("CAPABILITY", "Type mismatch: has %u, required %u (slot %u)",
                 entry->type, required_type, slot);
        return false;
    }

    if ((entry->rights & required_rights) != required_rights) {
        klog_sec("CAPABILITY", "Rights violation: has 0x%x, required 0x%x (slot %u)",
                 entry->rights, required_rights, slot);
        return false;
    }

    return true;
}

int32_t cap_delegate(capability_table_t *src_table, uint32_t src_slot,
                     capability_table_t *dst_table, uint32_t rights_mask) {
    if (!src_table || !dst_table || src_slot >= MAX_CAPABILITIES_PER_PROCESS) return -1;

    capability_t *src_entry = &src_table->entries[src_slot];
    if (!src_entry->valid) return -1;

    // Check if source token has GRANT rights
    if (!(src_entry->rights & CAP_RIGHT_GRANT)) {
        klog_sec("CAPABILITY", "Delegation denied: Source lacks CAP_RIGHT_GRANT");
        return -1;
    }

    // Rights attenuation: destination can NEVER receive more rights than source has
    uint32_t delegated_rights = (src_entry->rights & rights_mask);

    return cap_create(dst_table, src_entry->type, delegated_rights, src_entry->object_id);
}

bool cap_revoke(capability_table_t *table, uint32_t slot) {
    if (!table || slot >= MAX_CAPABILITIES_PER_PROCESS) return false;
    if (!table->entries[slot].valid) return false;

    table->entries[slot].valid = false;
    table->entries[slot].type = CAP_TYPE_NULL;
    table->entries[slot].rights = 0;
    table->entries[slot].object_id = 0;
    table->entries[slot].badge = 0;
    table->count--;
    return true;
}
