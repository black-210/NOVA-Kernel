/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/threat_model.h
 * Description: Comprehensive Software & Physical Threat Model Constants & Policies.
 * License: Apache-2.0
 */

#ifndef NOVA_THREAT_MODEL_H
#define NOVA_THREAT_MODEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Threat Classifications */
typedef enum {
    THREAT_UNTRUSTED_USERSPACE_APP = 1,
    THREAT_MALICIOUS_DRIVER,
    THREAT_NETWORK_REMOTE_EXPLOIT,
    THREAT_DMA_PERIPHERAL_ATTACK,
    THREAT_PHYSICAL_COLD_BOOT,
    THREAT_PHYSICAL_FIRMWARE_TAMPER,
    THREAT_SIDE_CHANNEL_TIMING
} nova_threat_category_t;

/* Security Mitigation Status */
typedef enum {
    SEC_MITIGATION_GUARANTEED_BY_SOFTWARE = 1,
    SEC_MITIGATION_REQUIRES_HARDWARE_ROOT_OF_TRUST,
    SEC_MITIGATION_HEURISTIC_DEFENSE
} nova_mitigation_type_t;

/* Active Security Controls */
typedef struct {
    bool wx_memory_enforced;        /* W^X memory: Pages cannot be simultaneously writable and executable */
    bool nx_bit_enabled;            /* Hardware No-Execute bit in PML4/PDPT/PD/PT */
    bool smep_smap_active;          /* Supervisor Mode Execution/Access Prevention */
    bool stack_canaries_active;     /* Compiler stack cookie verification */
    bool aslr_entropy_active;       /* Kernel & user address space layout randomization */
    bool capability_sandbox_strict; /* Non-root processes cannot escalate without explicit cap token */
    bool iommu_dma_protection;      /* VT-d / AMD-Vi DMA transaction isolation */
    bool verified_boot_chain;       /* Cryptographic hash chain validation at boot */
} nova_security_posture_t;

void security_init_subsystem(void);
bool security_verify_pointer(const void *ptr, size_t size, bool is_write);
void security_audit_log(uint32_t event_id, uint32_t severity, const char *message);

#endif /* NOVA_THREAT_MODEL_H */
