/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/mm/vmm.c
 * Description: Virtual Memory Manager (4-Level x86_64 Paging with W^X and NX bit).
 * License: Apache-2.0
 */

#include "vmm.h"
#include "pmm.h"
#include "types.h"
#include "kprintf.h"
#include "cpu.h"

static pml4_t *kernel_pml4 = NULL;

#define PML4_INDEX(v) (((v) >> 39) & 0x1FF)
#define PDPT_INDEX(v) (((v) >> 30) & 0x1FF)
#define PD_INDEX(v)   (((v) >> 21) & 0x1FF)
#define PT_INDEX(v)   (((v) >> 12) & 0x1FF)

static uint64_t *get_or_create_table(uint64_t *parent_table, size_t index, uint64_t flags) {
    if (parent_table[index] & PAGE_PRESENT) {
        return (uint64_t *)(parent_table[index] & ~0xFFFULL);
    }
    void *new_table = pmm_alloc_page();
    if (!new_table) return NULL;
    kmemset(new_table, 0, PAGE_SIZE);

    parent_table[index] = (uint64_t)new_table | PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER);
    return (uint64_t *)new_table;
}

void vmm_init(void) {
    kernel_pml4 = (pml4_t *)pmm_alloc_page();
    kmemset(kernel_pml4, 0, PAGE_SIZE);

    // Identity-map first 64MB for kernel code, data, heap, and initial services
    for (uint64_t addr = 0; addr < 0x04000000; addr += PAGE_SIZE) {
        // Enforce W^X: if address is kernel text section (e.g. < 2MB), keep it read-only executable, otherwise RW NX
        uint64_t flags = (addr < 0x200000) ? PAGE_KERNEL_RX : PAGE_KERNEL_RW;
        vmm_map_page(kernel_pml4, addr, addr, flags);
    }

    vmm_switch_address_space(kernel_pml4);
    kprintf("[VMM] 4-Level x86_64 Paging active (PML4=0x%p, W^X and NX enforced)\n", kernel_pml4);
}

pml4_t *vmm_create_address_space(void) {
    pml4_t *pml4 = (pml4_t *)pmm_alloc_page();
    if (!pml4) return NULL;
    kmemset(pml4, 0, PAGE_SIZE);

    // Share kernel high mappings
    for (int i = 256; i < 512; i++) {
        pml4[i] = kernel_pml4[i];
    }
    return pml4;
}

void vmm_destroy_address_space(pml4_t *pml4) {
    if (!pml4 || pml4 == kernel_pml4) return;
    // Free user page tables (0..255)
    for (int i = 0; i < 256; i++) {
        if (pml4[i] & PAGE_PRESENT) {
            uint64_t *pdpt = (uint64_t *)(pml4[i] & ~0xFFFULL);
            for (int j = 0; j < 512; j++) {
                if (pdpt[j] & PAGE_PRESENT) {
                    uint64_t *pd = (uint64_t *)(pdpt[j] & ~0xFFFULL);
                    for (int k = 0; k < 512; k++) {
                        if (pd[k] & PAGE_PRESENT) {
                            uint64_t *pt = (uint64_t *)(pd[k] & ~0xFFFULL);
                            pmm_free_page(pt);
                        }
                    }
                    pmm_free_page(pd);
                }
            }
            pmm_free_page(pdpt);
        }
    }
    pmm_free_page(pml4);
}

bool vmm_map_page(pml4_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    if (!pml4) return false;

    // Strict W^X policy check: cannot be writable and executable simultaneously
    if ((flags & PAGE_WRITABLE) && !(flags & PAGE_NX)) {
        // Enforce NX on writable pages
        flags |= PAGE_NX;
    }

    uint64_t *pdpt = get_or_create_table((uint64_t *)pml4, PML4_INDEX(virt), flags);
    if (!pdpt) return false;

    uint64_t *pd = get_or_create_table(pdpt, PDPT_INDEX(virt), flags);
    if (!pd) return false;

    uint64_t *pt = get_or_create_table(pd, PD_INDEX(virt), flags);
    if (!pt) return false;

    pt[PT_INDEX(virt)] = (phys & ~0xFFFULL) | flags | PAGE_PRESENT;
    invlpg(virt);
    return true;
}

bool vmm_unmap_page(pml4_t *pml4, uint64_t virt) {
    if (!pml4) return false;

    if (!(pml4[PML4_INDEX(virt)] & PAGE_PRESENT)) return false;
    uint64_t *pdpt = (uint64_t *)(pml4[PML4_INDEX(virt)] & ~0xFFFULL);

    if (!(pdpt[PDPT_INDEX(virt)] & PAGE_PRESENT)) return false;
    uint64_t *pd = (uint64_t *)(pdpt[PDPT_INDEX(virt)] & ~0xFFFULL);

    if (!(pd[PD_INDEX(virt)] & PAGE_PRESENT)) return false;
    uint64_t *pt = (uint64_t *)(pd[PD_INDEX(virt)] & ~0xFFFULL);

    pt[PT_INDEX(virt)] = 0;
    invlpg(virt);
    return true;
}

uint64_t vmm_virt_to_phys(pml4_t *pml4, uint64_t virt) {
    if (!pml4) return 0;
    if (!(pml4[PML4_INDEX(virt)] & PAGE_PRESENT)) return 0;
    uint64_t *pdpt = (uint64_t *)(pml4[PML4_INDEX(virt)] & ~0xFFFULL);

    if (!(pdpt[PDPT_INDEX(virt)] & PAGE_PRESENT)) return 0;
    uint64_t *pd = (uint64_t *)(pdpt[PDPT_INDEX(virt)] & ~0xFFFULL);

    if (!(pd[PD_INDEX(virt)] & PAGE_PRESENT)) return 0;
    uint64_t *pt = (uint64_t *)(pd[PD_INDEX(virt)] & ~0xFFFULL);

    if (!(pt[PT_INDEX(virt)] & PAGE_PRESENT)) return 0;
    return (pt[PT_INDEX(virt)] & ~0xFFFULL) | (virt & 0xFFF);
}

void vmm_switch_address_space(pml4_t *pml4) {
    if (pml4) {
        write_cr3((uint64_t)pml4);
    }
}

pml4_t *vmm_get_kernel_pml4(void) {
    return kernel_pml4;
}
