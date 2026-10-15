/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/core/string.c
 * Description: Bounds-checked and safe memory/string routines.
 * License: Apache-2.0
 */

#include "kprintf.h"

size_t kstrlen(const char *s) {
    if (!s) return 0;
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

int kstrcmp(const char *s1, const char *s2) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int kstrncmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) return 0;
    while (n-- && *s1 && (*s1 == *s2)) {
        if (n == 0 || *s1 == '\0') break;
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char *kstrncpy(char *dest, const char *src, size_t n) {
    if (!dest || !src || n == 0) return dest;
    size_t i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return dest;
}

void *kmemset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void *kmemcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

int kmemcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}
