// utils.c
#include "utils.h"

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