/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/arch/x86_64/serial.c
 * Description: 16550 UART serial driver for kernel logging.
 * License: Apache-2.0
 */

#include "serial.h"
#include "cpu.h"

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);    // Disable all interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x01);    // Set divisor to 1 (lo byte) 115200 baud
    outb(COM1_PORT + 1, 0x00);    //                  (hi byte)
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

static int is_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

void serial_putc(char c) {
    while (is_transmit_empty() == 0);
    outb(COM1_PORT, (uint8_t)c);
}

void serial_puts(const char *str) {
    if (!str) return;
    while (*str) {
        if (*str == '\n') {
            serial_putc('\r');
        }
        serial_putc(*str);
        str++;
    }
}

void serial_write(const char *buf, size_t len) {
    if (!buf) return;
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            serial_putc('\r');
        }
        serial_putc(buf[i]);
    }
}

int serial_has_data(void) {
    return inb(COM1_PORT + 5) & 1;
}

char serial_read(void) {
    while (serial_has_data() == 0);
    return (char)inb(COM1_PORT);
}
