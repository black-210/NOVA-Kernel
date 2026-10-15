/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/mm/pmm.c
 * Description: Physical Memory Manager (Bitmap Page Frame Allocator).
 * License: Apache-2.0
 */

#include "pmm.h"
#include "kprintf.h"
#include "types.h"

static uint8_t *bitmap = NULL;
static uint64_t total_frames = 0;
static uint64_t free_frames = 0;
static uint64_t used_frames = 0;

static inline void bitmap_set(uint64_t frame) {
    bitmap[frame / 8] |= (1 << (frame % 8));
}

static inline void bitmap_clear(uint64_t frame) {
    bitmap[frame / 8] &= ~(1 << (frame % 8));
}

static inline bool bitmap_test(uint64_t frame) {
    return (bitmap[frame / 8] & (1 << (frame % 8))) != 0;
}

void pmm_init(uint64_t mem_size_bytes, uintptr_t kernel_end_addr) {
    total_frames = mem_size_bytes / PMM_PAGE_SIZE;
    uint64_t bitmap_size_bytes = ALIGN_UP(total_frames / 8, PMM_PAGE_SIZE);

    // Place bitmap immediately following kernel
    bitmap = (uint8_t *)ALIGN_UP(kernel_end_addr, PMM_PAGE_SIZE);

    // Initially mark everything as allocated
    kmemset(bitmap, 0xFF, bitmap_size_bytes);
    used_frames = total_frames;
    free_frames = 0;

    // Free all frames above kernel + bitmap
    uint64_t reserved_boundary = (uint64_t)bitmap + bitmap_size_bytes;
    uint64_t first_usable_frame = ALIGN_UP(reserved_boundary, PMM_PAGE_SIZE) / PMM_PAGE_SIZE;

    for (uint64_t f = first_usable_frame; f < total_frames; f++) {
        bitmap_clear(f);
        free_frames++;
        used_frames--;
    }

    kprintf("[PMM] Physical Memory: %u MB total, %u frames free, bitmap at 0x%p (%u KB)\n",
            (uint32_t)(mem_size_bytes / (1024 * 1024)),
            (uint32_t)free_frames,
            bitmap,
            (uint32_t)(bitmap_size_bytes / 1024));
}

void *pmm_alloc_page(void) {
    for (uint64_t f = 0; f < total_frames; f++) {
        if (!bitmap_test(f)) {
            bitmap_set(f);
            free_frames--;
            used_frames++;
            return (void *)(f * PMM_PAGE_SIZE);
        }
    }
    klog_err("PMM", "Physical memory exhausted! Out of memory.");
    return NULL;
}

void *pmm_alloc_pages(size_t count) {
    if (count == 0) return NULL;
    if (count == 1) return pmm_alloc_page();

    size_t consecutive = 0;
    uint64_t start_frame = 0;

    for (uint64_t f = 0; f < total_frames; f++) {
        if (!bitmap_test(f)) {
            if (consecutive == 0) start_frame = f;
            consecutive++;
            if (consecutive == count) {
                for (uint64_t i = start_frame; i < start_frame + count; i++) {
                    bitmap_set(i);
                }
                free_frames -= count;
                used_frames += count;
                return (void *)(start_frame * PMM_PAGE_SIZE);
            }
        } else {
            consecutive = 0;
        }
    }
    return NULL;
}

void pmm_free_page(void *addr) {
    uint64_t frame = (uint64_t)addr / PMM_PAGE_SIZE;
    if (frame < total_frames && bitmap_test(frame)) {
        bitmap_clear(frame);
        free_frames++;
        used_frames--;
    }
}

void pmm_free_pages(void *addr, size_t count) {
    uint64_t frame = (uint64_t)addr / PMM_PAGE_SIZE;
    for (size_t i = 0; i < count; i++) {
        pmm_free_page((void *)((frame + i) * PMM_PAGE_SIZE));
    }
}

uint64_t pmm_get_total_pages(void) { return total_frames; }
uint64_t pmm_get_free_pages(void)  { return free_frames; }
uint64_t pmm_get_used_pages(void)  { return used_frames; }
