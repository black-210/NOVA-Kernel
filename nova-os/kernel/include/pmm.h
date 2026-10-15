/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/pmm.h
 * Description: Physical Memory Manager (Bitmap Page Frame Allocator).
 * License: Apache-2.0
 */

#ifndef NOVA_PMM_H
#define NOVA_PMM_H

#include <stdint.h>
#include <stddef.h>

#define PMM_PAGE_SIZE 4096ULL

void pmm_init(uint64_t mem_size_bytes, uintptr_t kernel_end_addr);
void *pmm_alloc_page(void);
void *pmm_alloc_pages(size_t count);
void  pmm_free_page(void *addr);
void  pmm_free_pages(void *addr, size_t count);

uint64_t pmm_get_total_pages(void);
uint64_t pmm_get_free_pages(void);
uint64_t pmm_get_used_pages(void);

#endif /* NOVA_PMM_H */
