/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/serial.h
 * Description: 16550 UART serial driver for kernel debug logging and console output.
 * License: Apache-2.0
 */

#ifndef NOVA_SERIAL_H
#define NOVA_SERIAL_H

#include <stdint.h>
#include <stddef.h>

#define COM1_PORT 0x3F8

void serial_init(void);
void serial_putc(char c);
void serial_puts(const char *str);
void serial_write(const char *buf, size_t len);
char serial_read(void);
int  serial_has_data(void);

#endif /* NOVA_SERIAL_H */
