#ifndef VGA_H
#define VGA_H

#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

extern uint8_t cursor_row;
extern uint8_t cursor_col;
extern int vga_lock;

void print_char(char c, uint8_t row, uint8_t col);
void print_string(const char *str);
void scroll_screen(void);
void clear_screen_proper(void);

#endif