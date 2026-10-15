/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/core/kprintf.c
 * Description: Kernel formatted printer, structured logging, and panic engine.
 * License: Apache-2.0
 */

#include "kprintf.h"
#include "serial.h"
#include "cpu.h"
#include <stdarg.h>

static void print_number(uint64_t n, int base, int is_signed) {
    char buf[65];
    char digits[] = "0123456789ABCDEF";
    int i = 0;

    if (is_signed && (int64_t)n < 0) {
        serial_putc('-');
        n = (uint64_t)(-(int64_t)n);
    }

    if (n == 0) {
        serial_putc('0');
        return;
    }

    while (n > 0) {
        buf[i++] = digits[n % (uint64_t)base];
        n /= (uint64_t)base;
    }

    while (i > 0) {
        serial_putc(buf[--i]);
    }
}

void kprintf(const char *fmt, ...) {
    if (!fmt) return;
    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            if (*p == '\n') serial_putc('\r');
            serial_putc(*p);
            continue;
        }

        p++;
        switch (*p) {
            case 'c': {
                char c = (char)va_arg(args, int);
                serial_putc(c);
                break;
            }
            case 's': {
                const char *s = va_arg(args, const char *);
                serial_puts(s ? s : "(null)");
                break;
            }
            case 'd':
            case 'i': {
                int64_t val = va_arg(args, int);
                print_number((uint64_t)val, 10, 1);
                break;
            }
            case 'u': {
                uint64_t val = va_arg(args, unsigned int);
                print_number(val, 10, 0);
                break;
            }
            case 'x':
            case 'X': {
                uint64_t val = va_arg(args, unsigned long long);
                print_number(val, 16, 0);
                break;
            }
            case 'p': {
                uint64_t val = (uint64_t)va_arg(args, void *);
                serial_puts("0x");
                print_number(val, 16, 0);
                break;
            }
            case '%': {
                serial_putc('%');
                break;
            }
            default: {
                serial_putc('%');
                serial_putc(*p);
                break;
            }
        }
    }

    va_end(args);
}

void klog_info(const char *subsystem, const char *fmt, ...) {
    kprintf("[INFO] [%s] ", subsystem ? subsystem : "KERNEL");
    va_list args;
    va_start(args, fmt);
    kprintf(fmt);
    va_end(args);
    kprintf("\n");
}

void klog_warn(const char *subsystem, const char *fmt, ...) {
    kprintf("[WARN] [%s] ", subsystem ? subsystem : "KERNEL");
    kprintf(fmt);
    kprintf("\n");
}

void klog_err(const char *subsystem, const char *fmt, ...) {
    kprintf("[ERROR] [%s] ", subsystem ? subsystem : "KERNEL");
    kprintf(fmt);
    kprintf("\n");
}

void klog_sec(const char *subsystem, const char *fmt, ...) {
    kprintf("[SEC-AUDIT] [%s] ", subsystem ? subsystem : "SECURITY");
    kprintf(fmt);
    kprintf("\n");
}

void kernel_panic(const char *reason, const char *file, int line) {
    cli();
    kprintf("\n=======================================================\n");
    kprintf("                 *** NOVA KERNEL PANIC ***              \n");
    kprintf("=======================================================\n");
    kprintf("Reason: %s\n", reason ? reason : "Unknown Fatal Condition");
    kprintf("Location: %s:%d\n", file ? file : "unknown", line);
    kprintf("CPU State: CR0=0x%x  CR2(FaultAddr)=0x%x  CR3=0x%x\n",
            read_cr0(), read_cr2(), read_cr3());
    kprintf("Execution halted safely. Tamper prevention active.\n");
    kprintf("=======================================================\n");

    while (1) {
        hlt();
    }
}
