// utils.c
#include "utils.h"
#include "vga.h" // Assuming print_string is defined in vga.h

void itoa(uint64_t val, char *buf, int base) {
    char *p = buf;
    if (val == 0) {
        *p++ = '0';
        *p = '\0';
        return;
    }
    while (val) {
        *p++ = "0123456789ABCDEF"[val % base];
        val /= base;
    }
    *p = '\0';
    char *start = buf;
    char *end = p - 1;
    while (start < end) {
        char temp = *start;
        *start++ = *end;
        *end-- = temp;
    }
}

void print_hex(uint64_t val) {
    char buf[17]; // Enough for 64-bit hex (16 chars + null terminator)
    itoa(val, buf, 16);
    print_string("0x"); // Prefix with "0x" for hex
    print_string(buf);
}

void print_dec(uint64_t val) {
    char buf[21]; // Enough for 64-bit decimal (20 digits + null terminator)
    itoa(val, buf, 10);
    print_string(buf);
}

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

char *strcpy(char *dest, const char *src) {
    char *original_dest = dest;
    while (*src != '\0') {
        *dest++ = *src++;
    }
    *dest = '\0'; // Null-terminate the destination
    return original_dest;
}

int strcmp(const char *str1, const char *str2) {
    if (!str1 || !str2) return -1;
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

int starts_with(const char *str, const char *prefix) {
    if (!str || !prefix) return 0;
    while (*prefix) {
        if (*str != *prefix) return 0;
        str++;
        prefix++;
    }
    return 1;
}

int string_length(const char *str) {
    if (!str) return 0;
    int len = 0;
    while (str[len] && len < 1000) len++;
    return len;
}

void string_copy(char *dest, const char *src, int max_input) {
    if (!dest || !src) return;
    while (*src && dest - dest < max_input - 1) {
        *dest++ = *src++;
    }
    *dest = '\0';
}