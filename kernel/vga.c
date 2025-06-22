#include "vga.h"
#include "task.h"

uint8_t cursor_row = 0;
uint8_t cursor_col = 0;
static volatile uint16_t *vga_buffer = (uint16_t *)0xB8000;
int vga_lock = 0;

void print_char(char c, uint8_t row, uint8_t col) {
    if (row >= VGA_HEIGHT || col >= VGA_WIDTH) return;
    enter_critical_section();
    int index = row * VGA_WIDTH + col;
    vga_buffer[index] = (uint16_t)(c | (0x0F << 8));
    exit_critical_section();
}

void print_string(const char *str) {
    enter_critical_section();
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cursor_row++;
            cursor_col = 0;
            if (cursor_row >= VGA_HEIGHT) {
                // Scroll screen up instead of wrapping to top
                scroll_screen();
                cursor_row = VGA_HEIGHT - 1;
            }
        } else {
            print_char(str[i], cursor_row, cursor_col);
            cursor_col++;
            if (cursor_col >= VGA_WIDTH) {
                cursor_row++;
                cursor_col = 0;
                if (cursor_row >= VGA_HEIGHT) {
                    scroll_screen();
                    cursor_row = VGA_HEIGHT - 1;
                }
            }
        }
    }
    exit_critical_section();
}

void scroll_screen(void) {
    // Move all lines up by one
    for (int row = 0; row < VGA_HEIGHT - 1; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            int src_index = (row + 1) * VGA_WIDTH + col;
            int dst_index = row * VGA_WIDTH + col;
            vga_buffer[dst_index] = vga_buffer[src_index];
        }
    }
    
    // Clear the last line
    for (int col = 0; col < VGA_WIDTH; col++) {
        int index = (VGA_HEIGHT - 1) * VGA_WIDTH + col;
        vga_buffer[index] = (uint16_t)(' ' | (0x0F << 8));
    }
}

void clear_screen_proper(void) {
    for (int row = 0; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            int index = row * VGA_WIDTH + col;
            vga_buffer[index] = (uint16_t)(' ' | (0x0F << 8));
        }
    }
    cursor_row = 0;
    cursor_col = 0;
}