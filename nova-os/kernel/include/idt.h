/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/idt.h
 * Description: Interrupt Descriptor Table and Exception Handling.
 * License: Apache-2.0
 */

#ifndef NOVA_IDT_H
#define NOVA_IDT_H

#include <stdint.h>

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* Interrupt / Exception Stack Frame pushed by CPU & stub */
struct interrupt_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

typedef void (*irq_handler_t)(struct interrupt_frame *frame);

void idt_init(void);
void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags);
void register_irq_handler(uint8_t irq, irq_handler_t handler);

#endif /* NOVA_IDT_H */
