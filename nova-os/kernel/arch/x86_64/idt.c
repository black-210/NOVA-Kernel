/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/arch/x86_64/idt.c
 * Description: Interrupt Descriptor Table, Exception Dispatch, and Page Fault Diagnostics.
 * License: Apache-2.0
 */

#include "idt.h"
#include "gdt.h"
#include "kprintf.h"
#include "cpu.h"
#include "sched.h"

extern void pic_remap(uint8_t o1, uint8_t o2);
extern void pic_send_eoi(uint8_t irq);
extern void pit_tick_handler(void);

static struct idt_entry idt_entries[256];
static struct idt_ptr   idt_pointer;
static irq_handler_t    irq_handlers[16];

// Assembly ISR extern declarations
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);  extern void isr3(void);
extern void isr4(void);  extern void isr5(void);  extern void isr6(void);  extern void isr7(void);
extern void isr8(void);  extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);
extern void isr32(void); extern void isr33(void); extern void isr34(void); extern void isr35(void);
extern void isr36(void); extern void isr37(void); extern void isr38(void); extern void isr39(void);
extern void isr40(void); extern void isr41(void); extern void isr42(void); extern void isr43(void);
extern void isr44(void); extern void isr45(void); extern void isr46(void); extern void isr47(void);
extern void isr128(void);

static const char *exception_messages[] = {
    "Division By Zero",
    "Debug Trap",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault (#GP)",
    "Page Fault (#PF)",
    "Unknown Interrupt",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor Injection", "VMM Communication", "Security Exception", "Reserved"
};

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].offset_low = (uint16_t)(base & 0xFFFF);
    idt_entries[num].selector = sel;
    idt_entries[num].ist = 0;
    idt_entries[num].type_attr = flags;
    idt_entries[num].offset_mid = (uint16_t)((base >> 16) & 0xFFFF);
    idt_entries[num].offset_high = (uint32_t)((base >> 32) & 0xFFFFFFFF);
    idt_entries[num].zero = 0;
}

void register_irq_handler(uint8_t irq, irq_handler_t handler) {
    if (irq < 16) {
        irq_handlers[irq] = handler;
    }
}

void isr_common_handler(struct interrupt_frame *frame) {
    if (frame->int_no < 32) {
        // CPU Exception
        if (frame->int_no == 14) {
            uint64_t fault_addr = read_cr2();
            kprintf("\n[PAGE FAULT] at RIP=0x%p accessing 0x%p (err=0x%x: %s%s%s%s)\n",
                    frame->rip, fault_addr, frame->err_code,
                    (frame->err_code & 1) ? "protection " : "not-present ",
                    (frame->err_code & 2) ? "write " : "read ",
                    (frame->err_code & 4) ? "user " : "kernel ",
                    (frame->err_code & 16) ? "instruction-fetch" : "data");
        } else {
            kprintf("\n[CPU EXCEPTION %u: %s] at RIP=0x%p, Error Code=0x%x\n",
                    frame->int_no,
                    frame->int_no < 32 ? exception_messages[frame->int_no] : "Unknown",
                    frame->rip, frame->err_code);
        }
        KERNEL_PANIC("Unhandled CPU Fault in Protected Execution Domain");
    } else if (frame->int_no >= 32 && frame->int_no <= 47) {
        // Hardware IRQ
        uint8_t irq = (uint8_t)(frame->int_no - 32);
        if (irq == 0) {
            pit_tick_handler();
            sched_tick();
        } else if (irq_handlers[irq]) {
            irq_handlers[irq](frame);
        }
        pic_send_eoi(irq);
    } else if (frame->int_no == 128) {
        // System Call (int 0x80)
        extern uint64_t syscall_dispatch(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
        frame->rax = syscall_dispatch(frame->rax, frame->rdi, frame->rsi, frame->rdx, frame->r10, frame->r8);
    }
}

void idt_init(void) {
    kmemset(&idt_entries, 0, sizeof(idt_entries));
    kmemset(&irq_handlers, 0, sizeof(irq_handlers));

    pic_remap(32, 40);

    // Exceptions 0..31 (0x8E = Present | Ring 0 | 64-bit Interrupt Gate)
    #define SET_GATE(i) idt_set_gate(i, (uint64_t)isr##i, KERNEL_CS, 0x8E)
    SET_GATE(0);  SET_GATE(1);  SET_GATE(2);  SET_GATE(3);
    SET_GATE(4);  SET_GATE(5);  SET_GATE(6);  SET_GATE(7);
    SET_GATE(8);  SET_GATE(9);  SET_GATE(10); SET_GATE(11);
    SET_GATE(12); SET_GATE(13); SET_GATE(14); SET_GATE(15);
    SET_GATE(16); SET_GATE(17); SET_GATE(18); SET_GATE(19);
    SET_GATE(20); SET_GATE(21); SET_GATE(22); SET_GATE(23);
    SET_GATE(24); SET_GATE(25); SET_GATE(26); SET_GATE(27);
    SET_GATE(28); SET_GATE(29); SET_GATE(30); SET_GATE(31);

    // IRQs 32..47
    SET_GATE(32); SET_GATE(33); SET_GATE(34); SET_GATE(35);
    SET_GATE(36); SET_GATE(37); SET_GATE(38); SET_GATE(39);
    SET_GATE(40); SET_GATE(41); SET_GATE(42); SET_GATE(43);
    SET_GATE(44); SET_GATE(45); SET_GATE(46); SET_GATE(47);

    // Syscall gate 0x80 (0xEE = Present | Ring 3 Accessible | Interrupt Gate)
    idt_set_gate(128, (uint64_t)isr128, KERNEL_CS, 0xEE);

    idt_pointer.limit = sizeof(idt_entries) - 1;
    idt_pointer.base = (uint64_t)&idt_entries;

    __asm__ volatile ("lidt %0" : : "m"(idt_pointer) : "memory");
    kprintf("[IDT] 256-entry IDT loaded at 0x%p (Exceptions, IRQs 0-15, Syscall 0x80 registered)\n", &idt_entries);
}
