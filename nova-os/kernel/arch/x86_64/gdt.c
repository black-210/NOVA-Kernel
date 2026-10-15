/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/arch/x86_64/gdt.c
 * Description: GDT64 and TSS Setup with Ring 0 & Ring 3 Descriptors.
 * License: Apache-2.0
 */

#include "gdt.h"
#include "kprintf.h"
#include "cpu.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct tss_descriptor {
    uint16_t length;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  flags1;
    uint8_t  flags2;
    uint8_t  base_high;
    uint32_t base_upper32;
    uint32_t reserved;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct {
    struct gdt_entry null_desc;
    struct gdt_entry kernel_code;
    struct gdt_entry kernel_data;
    struct gdt_entry user_data;
    struct gdt_entry user_code;
    struct tss_descriptor tss_desc;
} __attribute__((packed)) gdt_table;

static struct tss_entry kernel_tss;
static struct gdt_ptr   gdt_pointer;

extern void gdt_flush_asm(uint64_t ptr);
extern void tss_flush_asm(uint16_t sel);

void gdt_init(void) {
    kmemset(&gdt_table, 0, sizeof(gdt_table));
    kmemset(&kernel_tss, 0, sizeof(kernel_tss));

    // Kernel Code 64-bit (0x08): Read/Exec, Ring 0, Long mode
    gdt_table.kernel_code.access = 0x9A;      // Present | Executable | Read
    gdt_table.kernel_code.granularity = 0x20; // 64-bit long mode flag

    // Kernel Data (0x10): Read/Write, Ring 0
    gdt_table.kernel_data.access = 0x92;      // Present | Writable

    // User Data (0x18, RPL=3 -> 0x1B): Read/Write, Ring 3
    gdt_table.user_data.access = 0xF2;        // Present | Ring 3 | Writable

    // User Code 64-bit (0x20, RPL=3 -> 0x23): Read/Exec, Ring 3, Long mode
    gdt_table.user_code.access = 0xFA;        // Present | Ring 3 | Executable | Read
    gdt_table.user_code.granularity = 0x20;   // 64-bit long mode flag

    // TSS Descriptor (0x28)
    uint64_t tss_base = (uint64_t)&kernel_tss;
    uint32_t tss_limit = sizeof(kernel_tss) - 1;

    gdt_table.tss_desc.length = (uint16_t)(tss_limit & 0xFFFF);
    gdt_table.tss_desc.base_low = (uint16_t)(tss_base & 0xFFFF);
    gdt_table.tss_desc.base_mid = (uint8_t)((tss_base >> 16) & 0xFF);
    gdt_table.tss_desc.flags1 = 0x89; // Present | 64-bit TSS (Available)
    gdt_table.tss_desc.flags2 = 0x00;
    gdt_table.tss_desc.base_high = (uint8_t)((tss_base >> 24) & 0xFF);
    gdt_table.tss_desc.base_upper32 = (uint32_t)(tss_base >> 32);
    gdt_table.tss_desc.reserved = 0;

    kernel_tss.iomap_base = sizeof(kernel_tss);

    gdt_pointer.limit = sizeof(gdt_table) - 1;
    gdt_pointer.base = (uint64_t)&gdt_table;

    __asm__ volatile (
        "lgdt %0\n"
        "ltr  %%ax\n"
        :
        : "m"(gdt_pointer), "a"((uint16_t)TSS_SEG)
        : "memory"
    );

    kprintf("[GDT] 64-bit GDT & TSS initialized at 0x%p (TSS Stack base configured)\n", &gdt_table);
}

void tss_set_stack(uint64_t kernel_stack) {
    kernel_tss.rsp0 = kernel_stack;
}
