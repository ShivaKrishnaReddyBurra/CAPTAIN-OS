#include "vga.h"

uint8_t cursor_row = 0;
uint8_t cursor_col = 0;
static volatile uint16_t *vga_buffer = (uint16_t *)0xB8000;

void print_char(char c, uint8_t row, uint8_t col) {
    if (row >= VGA_HEIGHT || col >= VGA_WIDTH) return;
    int index = row * VGA_WIDTH + col;
    vga_buffer[index] = (uint16_t)(c | (0x0F << 8));
}

void print_string(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cursor_row++;
            cursor_col = 0;
            if (cursor_row >= VGA_HEIGHT) {
                cursor_row = 0;
                cursor_col = 0;
            }
        } else {
            print_char(str[i], cursor_row, cursor_col);
            cursor_col++;
            if (cursor_col >= VGA_WIDTH) {
                cursor_row++;
                cursor_col = 0;
                if (cursor_row >= VGA_HEIGHT) {
                    cursor_row = 0;
                    cursor_col = 0;
                }
            }
        }
    }
}