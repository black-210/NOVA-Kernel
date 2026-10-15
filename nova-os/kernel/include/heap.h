/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/heap.h
 * Description: Hardened Kernel Heap Allocator with Canaries and Double-Free Detection.
 * License: Apache-2.0
 */

#ifndef NOVA_HEAP_H
#define NOVA_HEAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define HEAP_MAGIC_ALLOCATED 0x4E4F5641414C4C43ULL /* 'NOVAALLC' */
#define HEAP_MAGIC_FREE      0x4E4F564146524545ULL /* 'NOVAFREE' */
#define HEAP_CANARY_VALUE    0xDEADC0DEBEEFCAFEULL

void  heap_init(uint64_t start_addr, size_t size_bytes);
void *kmalloc(size_t size);
void *kzalloc(size_t size);
void  kfree(void *ptr);
bool  heap_validate_integrity(void);
size_t heap_get_used_bytes(void);
size_t heap_get_free_bytes(void);

#endif /* NOVA_HEAP_H */
