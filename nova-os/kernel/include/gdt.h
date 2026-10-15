/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/gdt.h
 * Description: x86_64 Global Descriptor Table and Task State Segment.
 * License: Apache-2.0
 */

#ifndef NOVA_GDT_H
#define NOVA_GDT_H

#include <stdint.h>

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define USER_CS   0x1B /* Ring 3 with RPL=3 */
#define USER_DS   0x23 /* Ring 3 with RPL=3 */
#define TSS_SEG   0x28

struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

void gdt_init(void);
void tss_set_stack(uint64_t kernel_stack);

#endif /* NOVA_GDT_H */
