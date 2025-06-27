// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h> 

void itoa(uint64_t val, char *buf, int base);
void print_hex(uint64_t val);
void print_dec(uint64_t val);
size_t strlen(const char *str);
char *strcpy(char *dest, const char *src);
int strcmp(const char *str1, const char *str2);
int starts_with(const char *str, const char *prefix);
int string_length(const char *str);
void string_copy(char *dest, const char *src, int max_input);



#endif