#include "idt.h"
#include "vga.h"
#include "utils.h"

#define MAX_INPUT 80

// Shell state
char input_buffer[MAX_INPUT];
int input_pos = 0;

void clear_screen(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        ((uint16_t *)0xB8000)[i] = (uint16_t)(' ' | (0x0F << 8));
    }
    cursor_row = 0;
    cursor_col = 0;
}

void handle_command(void) {
    input_buffer[input_pos] = '\0'; // Null-terminate the input

    // Simple command parsing
    if (input_buffer[0] == 'c' && input_buffer[1] == 'l' && input_buffer[2] == 'e' && input_buffer[3] == 'a' && input_buffer[4] == 'r' && input_buffer[5] == '\0') {
        clear_screen();
    } else if (input_buffer[0] == 'e' && input_buffer[1] == 'c' && input_buffer[2] == 'h' && input_buffer[3] == 'o' && input_buffer[4] == ' ') {
        print_string("\n");
        print_string(&input_buffer[5]);
        print_string("\n");
    } else if(input_buffer[0] == 'o' && input_buffer[1] == 's' && input_buffer[2] == '\0') {
        print_string("\nAPTAIN-OS Kernel v1.0\n");
    } else if(input_buffer[0] == 'h' && input_buffer[1] == 'e' && input_buffer[2] == 'l' && input_buffer[3] == 'p' && input_buffer[4] == '\0') {
        print_string("\nAvailable commands:\n");
        print_string("clear - Clear the screen\n");
        print_string("echo <text> - Print text to the screen\n");
        print_string("os - Display OS information\n");
        print_string("help - Display this help message\n");
    } else if (input_buffer[0] == 'a' && input_buffer[1] == 'n' && input_buffer[2] == 'i' && input_buffer[3] == 'm' && input_buffer[4] == 'e' && input_buffer[5] == '\0'){
        print_string("\n");
        print_string("  .--.   .--.\n");
        print_string(" (o  o) (o  o)\n");
        print_string("  |  |   |  |\n");
        print_string("  '--'   '--'\n");
        print_string("\n");
    } else if (input_buffer[0] != '\0') {
        print_string("\nUnknown command: ");
        print_string(input_buffer);
        print_string("\n");
    }

    // Reset input buffer
    input_pos = 0;
    for (int i = 0; i < MAX_INPUT; i++) {
        input_buffer[i] = 0;
    }

    // Print prompt
    print_string("> ");
}

void kernel_main(void) {
    // Disable interrupts first
    __asm__ volatile("cli");
    
    clear_screen();

    print_string("Welcome to CAPTAIN-OS!\n");
    print_string("Commands: clear, echo <text>\n");
    print_string("\n> ");

    // Set up interrupt handling
    idt_init();
    pic_remap();
    enable_keyboard();

    // Enable interrupts
    __asm__ volatile("sti");

    // Main kernel loop
    while (1) {
        __asm__ volatile("hlt");
    }
}