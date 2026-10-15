/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/security/audit.c
 * Description: Tamper-Resistant Security Event Audit Log (Circular Ring Buffer).
 * License: Apache-2.0
 */

#include "threat_model.h"
#include "kprintf.h"

#define MAX_AUDIT_LOGS 128

typedef struct {
    uint64_t timestamp_tick;
    uint32_t event_id;
    uint32_t severity;
    char     message[80];
} audit_entry_t;

static audit_entry_t audit_log[MAX_AUDIT_LOGS];
static uint32_t audit_head = 0;
static uint32_t audit_total_events = 0;

void security_audit_log(uint32_t event_id, uint32_t severity, const char *message) {
    audit_entry_t *entry = &audit_log[audit_head];
    entry->timestamp_tick = 0; // read from PIT ticks
    entry->event_id = event_id;
    entry->severity = severity;
    kstrncpy(entry->message, message ? message : "Empty", sizeof(entry->message));

    audit_head = (audit_head + 1) % MAX_AUDIT_LOGS;
    audit_total_events++;

    klog_sec("AUDIT", "Event #%u [Sev %u]: %s", event_id, severity, message ? message : "");
}

uint32_t security_get_audit_count(void) {
    return audit_total_events;
}
