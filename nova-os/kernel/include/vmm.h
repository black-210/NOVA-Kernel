/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/vmm.h
 * Description: Virtual Memory Manager (4-Level x86_64 Paging with W^X enforcement).
 * License: Apache-2.0
 */

#ifndef NOVA_VMM_H
#define NOVA_VMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_PRESENT  (1ULL << 0)
#define PAGE_WRITABLE (1ULL << 1)
#define PAGE_USER     (1ULL << 2)
#define PAGE_NX       (1ULL << 63) /* No-Execute bit */

/* W^X Rule: Memory cannot be writable and executable simultaneously */
#define PAGE_KERNEL_RO    (PAGE_PRESENT)
#define PAGE_KERNEL_RW    (PAGE_PRESENT | PAGE_WRITABLE | PAGE_NX)
#define PAGE_KERNEL_RX    (PAGE_PRESENT)
#define PAGE_USER_RO      (PAGE_PRESENT | PAGE_USER)
#define PAGE_USER_RW      (PAGE_PRESENT | PAGE_USER | PAGE_WRITABLE | PAGE_NX)
#define PAGE_USER_RX      (PAGE_PRESENT | PAGE_USER)

typedef uint64_t pml4_t;

void vmm_init(void);
pml4_t *vmm_create_address_space(void);
void vmm_destroy_address_space(pml4_t *pml4);
bool vmm_map_page(pml4_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags);
bool vmm_unmap_page(pml4_t *pml4, uint64_t virt);
uint64_t vmm_virt_to_phys(pml4_t *pml4, uint64_t virt);
void vmm_switch_address_space(pml4_t *pml4);
pml4_t *vmm_get_kernel_pml4(void);

#endif /* NOVA_VMM_H */
