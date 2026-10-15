/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/mm/heap.c
 * Description: Hardened Kernel Heap Allocator with Magic Verification & Canaries.
 * Mitigates Use-After-Free, Double-Free, and Heap Metadata Overwrites.
 * License: Apache-2.0
 */

#include "heap.h"
#include "kprintf.h"
#include "types.h"

typedef struct heap_block {
    uint64_t magic;          /* HEAP_MAGIC_ALLOCATED or HEAP_MAGIC_FREE */
    size_t   size;           /* Size of user payload */
    uint64_t canary;         /* Canary word for integrity checking */
    struct heap_block *next;
    struct heap_block *prev;
} heap_block_t;

static uint8_t *heap_base = NULL;
static size_t   heap_total_size = 0;
static size_t   heap_used_bytes = 0;
static heap_block_t *free_list = NULL;

void heap_init(uint64_t start_addr, size_t size_bytes) {
    heap_base = (uint8_t *)start_addr;
    heap_total_size = size_bytes;
    heap_used_bytes = 0;

    free_list = (heap_block_t *)heap_base;
    free_list->magic = HEAP_MAGIC_FREE;
    free_list->size = size_bytes - sizeof(heap_block_t);
    free_list->canary = HEAP_CANARY_VALUE;
    free_list->next = NULL;
    free_list->prev = NULL;

    kprintf("[HEAP] Kernel Heap initialized at 0x%p (%u KB), canary=0x%x\n",
            heap_base, (uint32_t)(size_bytes / 1024), HEAP_CANARY_VALUE);
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    size = ALIGN_UP(size, 16);

    heap_block_t *curr = free_list;
    while (curr) {
        if (curr->magic != HEAP_MAGIC_FREE) {
            KERNEL_PANIC("Heap corruption detected: Invalid magic in free list block");
        }
        if (curr->canary != HEAP_CANARY_VALUE) {
            KERNEL_PANIC("Heap corruption detected: Canary word overwrite in free block");
        }

        if (curr->size >= size) {
            // Check if we can split this block
            if (curr->size >= size + sizeof(heap_block_t) + 32) {
                heap_block_t *remainder = (heap_block_t *)((uint8_t *)curr + sizeof(heap_block_t) + size);
                remainder->magic = HEAP_MAGIC_FREE;
                remainder->size = curr->size - size - sizeof(heap_block_t);
                remainder->canary = HEAP_CANARY_VALUE;
                remainder->next = curr->next;
                remainder->prev = curr->prev;

                if (curr->prev) curr->prev->next = remainder;
                else free_list = remainder;

                if (curr->next) curr->next->prev = remainder;

                curr->size = size;
            } else {
                // Take entire block
                if (curr->prev) curr->prev->next = curr->next;
                else free_list = curr->next;

                if (curr->next) curr->next->prev = curr->prev;
            }

            curr->magic = HEAP_MAGIC_ALLOCATED;
            curr->canary = HEAP_CANARY_VALUE;
            curr->next = NULL;
            curr->prev = NULL;

            heap_used_bytes += curr->size + sizeof(heap_block_t);
            return (void *)((uint8_t *)curr + sizeof(heap_block_t));
        }
        curr = curr->next;
    }

    klog_err("HEAP", "Out of heap memory allocating %u bytes", (uint32_t)size);
    return NULL;
}

void *kzalloc(size_t size) {
    void *ptr = kmalloc(size);
    if (ptr) {
        kmemset(ptr, 0, size);
    }
    return ptr;
}

void kfree(void *ptr) {
    if (!ptr) return;

    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));

    // Double-free / Use-After-Free Detection
    if (block->magic == HEAP_MAGIC_FREE) {
        KERNEL_PANIC("Security Fault: Double-Free detected on heap pointer!");
    }

    if (block->magic != HEAP_MAGIC_ALLOCATED) {
        KERNEL_PANIC("Security Fault: Invalid or corrupted heap header during kfree!");
    }

    if (block->canary != HEAP_CANARY_VALUE) {
        KERNEL_PANIC("Security Fault: Buffer Overflow detected! Heap Canary clobbered.");
    }

    // Mark as free and return to free list head
    block->magic = HEAP_MAGIC_FREE;
    heap_used_bytes -= (block->size + sizeof(heap_block_t));

    // Scramble user payload to prevent stale pointer re-use (Zero-Fill)
    kmemset(ptr, 0x55, block->size);

    block->next = free_list;
    block->prev = NULL;
    if (free_list) {
        free_list->prev = block;
    }
    free_list = block;
}

bool heap_validate_integrity(void) {
    heap_block_t *curr = free_list;
    while (curr) {
        if (curr->magic != HEAP_MAGIC_FREE || curr->canary != HEAP_CANARY_VALUE) {
            return false;
        }
        curr = curr->next;
    }
    return true;
}

size_t heap_get_used_bytes(void) { return heap_used_bytes; }
size_t heap_get_free_bytes(void) { return heap_total_size > heap_used_bytes ? heap_total_size - heap_used_bytes : 0; }
