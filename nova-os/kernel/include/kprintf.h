/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/include/kprintf.h
 * Description: Safe kernel formatted logging and panic handling.
 * License: Apache-2.0
 */

#ifndef NOVA_KPRINTF_H
#define NOVA_KPRINTF_H

#include <stddef.h>
#include <stdint.h>

void kprintf(const char *fmt, ...);
void klog_info(const char *subsystem, const char *fmt, ...);
void klog_warn(const char *subsystem, const char *fmt, ...);
void klog_err(const char *subsystem, const char *fmt, ...);
void klog_sec(const char *subsystem, const char *fmt, ...);

void kernel_panic(const char *reason, const char *file, int line) __attribute__((noreturn));
#define KERNEL_PANIC(msg) kernel_panic(msg, __FILE__, __LINE__)

/* Safe string functions */
size_t kstrlen(const char *s);
int    kstrcmp(const char *s1, const char *s2);
int    kstrncmp(const char *s1, const char *s2, size_t n);
char  *kstrncpy(char *dest, const char *src, size_t n);
void  *kmemset(void *s, int c, size_t n);
void  *kmemcpy(void *dest, const void *src, size_t n);
int    kmemcmp(const void *s1, const void *s2, size_t n);

#endif /* NOVA_KPRINTF_H */
