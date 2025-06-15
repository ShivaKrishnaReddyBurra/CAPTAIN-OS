#include "idt.h"
#include "vga.h"
#include "utils.h"

// Shell state (defined in kernel.c)
extern char input_buffer[80];  // Assuming MAX_INPUT is 80
extern int input_pos;
extern void handle_command(void);

// Correct PS/2 keyboard scancode to ASCII mapping
char scancode_to_ascii(uint8_t scancode) {
    switch (scancode) {
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';
        case 0x0C: return '-';
        case 0x0D: return '=';
        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x1A: return '[';
        case 0x1B: return ']';
        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x27: return ';';
        case 0x28: return '\'';
        case 0x29: return '`';
        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';
        case 0x33: return ',';
        case 0x34: return '.';
        case 0x35: return '/';
        case 0x2B: return '\\';
        case 0x39: return ' ';
        case 0x1C: return '\n';
        case 0x0E: return '\b';
        case 0x0F: return '\t';
        default: return 0;
    }
}

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    
    if (!(scancode & 0x80)) {  // Key press
        char c = scancode_to_ascii(scancode);
        if (c != 0) {
            if (c == '\n') {
                handle_command();
            } else if (c == '\b') {
                if (input_pos > 0) {
                    input_pos--;
                    if (cursor_col > 0) {
                        cursor_col--;
                        print_char(' ', cursor_row, cursor_col);
                    } else if (cursor_row > 0) {
                        cursor_row--;
                        cursor_col = VGA_WIDTH - 1;
                        print_char(' ', cursor_row, cursor_col);
                    }
                }
            } else if (input_pos < 80 - 1) {  // Assuming MAX_INPUT is 80
                input_buffer[input_pos++] = c;
                print_char(c, cursor_row, cursor_col);
                cursor_col++;
                if (cursor_col >= VGA_WIDTH) {
                    cursor_row++;
                    cursor_col = 0;
                    if (cursor_row >= VGA_HEIGHT) {
                        cursor_row = 0;
                    }
                }
            }
        }
    }
    
    pic_send_eoi(1);
}

void enable_keyboard(void) {
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
    
    int timeout = 10000;
    while ((inb(0x64) & 0x02) && timeout--) {
        // Wait for input buffer to be empty
    }
    
    if (timeout <= 0) {
        print_string("Keyboard timeout!\n");
        return;
    }
    
    outb(0x60, 0xF4);
    
    timeout = 10000;
    while (!(inb(0x64) & 0x01) && timeout--);
    if (timeout > 0) {
        uint8_t ack = inb(0x60);
        if (ack == 0xFA) {
            print_string("Keyboard ready.\n");
        } else {
            print_string("Keyboard error.\n");
        }
    }
}

void timer_handler(void) {
    static int timer_ticks = 0;
    timer_ticks++;
    
    if (timer_ticks % 100 == 0) {
        print_char('.', 0, 79);
    }
    
    pic_send_eoi(0);
}

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idt_p;

extern void load_idt(uint64_t idt_ptr);
extern void keyboard_isr(void);
extern void exception_isr(void);
extern void timer_isr(void);

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_mid = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
    idt[num].reserved = 0;
}

void exception_handler(void) {
    print_string("General Protection Fault!\n");
    
    uint64_t error_code;
    __asm__ volatile("mov 8(%%rbp), %0" : "=r"(error_code));
    
    char buffer[16];
    print_string("Error code: ");
    itoa(error_code, buffer, 16);
    print_string(buffer);
    print_string("\n");
    
    print_string("System halted.\n");
    
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}

void idt_init(void) {
    idt_p.limit = sizeof(struct idt_entry) * IDT_ENTRIES - 1;
    idt_p.base = (uint64_t)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    idt_set_gate(0, (uint64_t)exception_isr, 0x08, 0x8E);
    idt_set_gate(13, (uint64_t)exception_isr, 0x08, 0x8E);
    idt_set_gate(14, (uint64_t)exception_isr, 0x08, 0x8E);
    idt_set_gate(0x20, (uint64_t)timer_isr, 0x08, 0x8E);
    idt_set_gate(0x21, (uint64_t)keyboard_isr, 0x08, 0x8E);

    load_idt((uint64_t)&idt_p);
}