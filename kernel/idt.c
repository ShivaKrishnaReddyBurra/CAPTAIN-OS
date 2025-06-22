#include "idt.h"
#include "vga.h"
#include "utils.h"
#include "task.h"

extern char input_buffer[80];
extern int input_pos;
extern void handle_command(void);

extern void enter_critical_section(void);
extern void exit_critical_section(void);
extern int is_in_critical_section(void);

static uint32_t timer_ticks = 0;
static int shift_pressed = 0;
static int caps_lock = 0;

char scancode_to_ascii(uint8_t scancode) {
    // Handle special keys first
    if (scancode == 0x2A || scancode == 0x36) {  // Left or right shift press
        shift_pressed = 1;
        return 0;
    } else if (scancode == 0xAA || scancode == 0xB6) {  // Shift release
        shift_pressed = 0;
        return 0;
    } else if (scancode == 0x3A) {  // Caps lock press
        caps_lock = !caps_lock;
        return 0;
    }

    if (scancode & 0x80) {  // Key release, ignore
        return 0;
    }

    // Determine if we should use uppercase for letters
    int use_upper = shift_pressed ^ caps_lock;  // XOR for caps lock behavior

    switch (scancode) {
        // Numbers
        case 0x02: return shift_pressed ? '!' : '1';
        case 0x03: return shift_pressed ? '@' : '2';
        case 0x04: return shift_pressed ? '#' : '3';
        case 0x05: return shift_pressed ? '$' : '4';
        case 0x06: return shift_pressed ? '%' : '5';
        case 0x07: return shift_pressed ? '^' : '6';
        case 0x08: return shift_pressed ? '&' : '7';
        case 0x09: return shift_pressed ? '*' : '8';
        case 0x0A: return shift_pressed ? '(' : '9';
        case 0x0B: return shift_pressed ? ')' : '0';
        case 0x0C: return shift_pressed ? '_' : '-';
        case 0x0D: return shift_pressed ? '+' : '=';
        
        // Letters - top row
        case 0x10: return use_upper ? 'Q' : 'q';
        case 0x11: return use_upper ? 'W' : 'w';
        case 0x12: return use_upper ? 'E' : 'e';
        case 0x13: return use_upper ? 'R' : 'r';
        case 0x14: return use_upper ? 'T' : 't';
        case 0x15: return use_upper ? 'Y' : 'y';
        case 0x16: return use_upper ? 'U' : 'u';
        case 0x17: return use_upper ? 'I' : 'i';
        case 0x18: return use_upper ? 'O' : 'o';
        case 0x19: return use_upper ? 'P' : 'p';
        case 0x1A: return shift_pressed ? '{' : '[';
        case 0x1B: return shift_pressed ? '}' : ']';
        
        // Letters - middle row
        case 0x1E: return use_upper ? 'A' : 'a';
        case 0x1F: return use_upper ? 'S' : 's';
        case 0x20: return use_upper ? 'D' : 'd';
        case 0x21: return use_upper ? 'F' : 'f';
        case 0x22: return use_upper ? 'G' : 'g';
        case 0x23: return use_upper ? 'H' : 'h';
        case 0x24: return use_upper ? 'J' : 'j';
        case 0x25: return use_upper ? 'K' : 'k';
        case 0x26: return use_upper ? 'L' : 'l';
        case 0x27: return shift_pressed ? ':' : ';';
        case 0x28: return shift_pressed ? '"' : '\'';
        case 0x29: return shift_pressed ? '~' : '`';
        
        // Letters - bottom row
        case 0x2C: return use_upper ? 'Z' : 'z';
        case 0x2D: return use_upper ? 'X' : 'x';
        case 0x2E: return use_upper ? 'C' : 'c';
        case 0x2F: return use_upper ? 'V' : 'v';
        case 0x30: return use_upper ? 'B' : 'b';
        case 0x31: return use_upper ? 'N' : 'n';
        case 0x32: return use_upper ? 'M' : 'm';
        case 0x33: return shift_pressed ? '<' : ',';
        case 0x34: return shift_pressed ? '>' : '.';
        case 0x35: return shift_pressed ? '?' : '/';
        case 0x2B: return shift_pressed ? '|' : '\\';
        
        // Special keys
        case 0x39: return ' ';      // Space
        case 0x1C: return '\n';     // Enter
        case 0x0E: return '\b';     // Backspace
        case 0x0F: return '\t';     // Tab
        
        default: return 0;
    }
}

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    char c = scancode_to_ascii(scancode);
    
    if (c != 0) {
        enter_critical_section();
        
        if (c == '\n') {
            print_char('\n', cursor_row, cursor_col);
            cursor_row++;
            cursor_col = 0;
            if (cursor_row >= VGA_HEIGHT) {
                scroll_screen();
                cursor_row = VGA_HEIGHT - 1;
            }
            handle_command();
        } else if (c == '\b') {
            if (input_pos > 0) {
                input_pos--;
                // Calculate proper cursor position for backspace
                if (cursor_col > 0) {
                    cursor_col--;
                } else if (cursor_row > 0) {
                    cursor_row--;
                    cursor_col = VGA_WIDTH - 1;
                }
                print_char(' ', cursor_row, cursor_col);
            }
        } else if (c == '\t') {
            // Handle tab - add 4 spaces or align to next tab stop
            int spaces = 4 - (cursor_col % 4);
            for (int i = 0; i < spaces && input_pos < 79; i++) {
                input_buffer[input_pos++] = ' ';
                print_char(' ', cursor_row, cursor_col);
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
        } else if (input_pos < 79) {  // Regular character
            input_buffer[input_pos++] = c;
            print_char(c, cursor_row, cursor_col);
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
        
        exit_critical_section();
    }
    
    pic_send_eoi(1);
}

void enable_keyboard(void) {
    // Clear keyboard buffer first
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }
    
    int attempts = 5;
    while (attempts--) {
        // Wait for keyboard controller to be ready
        int timeout = 65536;
        while ((inb(0x64) & 0x02) && --timeout > 0);
        
        if (timeout == 0) {
            print_string("Keyboard controller timeout, retrying...\n");
            continue;
        }
        
        // Send enable command
        outb(0x60, 0xF4);
        
        // Wait for response
        timeout = 65536;
        while (!(inb(0x64) & 0x01) && --timeout > 0);
        
        if (timeout > 0) {
            uint8_t response = inb(0x60);
            if (response == 0xFA) {  // ACK
                print_string("Keyboard enabled successfully.\n");
                return;
            } else if (response == 0xFE) {  // Resend
                print_string("Keyboard requested resend, retrying...\n");
                continue;
            } else {
                char buffer[16];
                print_string("Keyboard unexpected response: 0x");
                itoa(response, buffer, 16);
                print_string(buffer);
                print_string(", retrying...\n");
            }
        } else {
            print_string("Keyboard response timeout, retrying...\n");
        }
    }
    
    print_string("Failed to enable keyboard after multiple attempts.\n");
}

void timer_handler(void) {
    timer_ticks++;
    
    // Reduce timer-based scheduling frequency for smoother operation
    if (timer_ticks % 50 == 0) {  // Every 0.5 seconds at 100Hz
        if (!is_in_critical_section()) {
            schedule();
        }
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
    enter_critical_section();
    print_string("\n*** SYSTEM EXCEPTION ***\n");
    print_string("An exception has occurred!\n");
    
    char buffer[20];
    print_string("Current task ID: ");
    if (current_task) {
        itoa(current_task->id, buffer, 10);
        print_string(buffer);
    } else {
        print_string("None");
    }
    print_string("\n");
    
    print_string("Timer ticks: ");
    itoa(timer_ticks, buffer, 10);
    print_string(buffer);
    print_string("\n");
    
    print_string("System halted. Please restart.\n");
    
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}

void idt_init(void) {
    idt_p.limit = sizeof(struct idt_entry) * IDT_ENTRIES - 1;
    idt_p.base = (uint64_t)&idt;

    // Initialize all entries to zero
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // Set up exception handlers
    idt_set_gate(0, (uint64_t)exception_isr, 0x08, 0x8E);   // Division by zero
    idt_set_gate(6, (uint64_t)exception_isr, 0x08, 0x8E);   // Invalid opcode
    idt_set_gate(13, (uint64_t)exception_isr, 0x08, 0x8E);  // General protection fault
    idt_set_gate(14, (uint64_t)exception_isr, 0x08, 0x8E);  // Page fault
    
    // Set up interrupt handlers
    idt_set_gate(0x20, (uint64_t)timer_isr, 0x08, 0x8E);    // Timer interrupt
    idt_set_gate(0x21, (uint64_t)keyboard_isr, 0x08, 0x8E); // Keyboard interrupt

    load_idt((uint64_t)&idt_p);
}