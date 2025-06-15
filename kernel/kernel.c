#include "idt.h"
#include "vga.h"
#include "utils.h"

void clear_screen(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        ((uint16_t *)0xB8000)[i] = (uint16_t)(' ' | (0x0F << 8));
    }
    cursor_row = 0;
    cursor_col = 0;
}

void kernel_main(void) {
    // Disable interrupts first
    __asm__ volatile("cli");
    
    clear_screen();

    print_string("Welcome to CAPTAIN-OS!\n");

    char buffer[16]; // First declaration
    itoa((uint64_t)&cursor_row, buffer, 16);

    // Check initial interrupt state
    uint64_t flags;
    __asm__ volatile("pushfq; pop %0" : "=r"(flags));
    if (flags & (1 << 9)) {
        print_string("Interrupts enabled (initial).\n");
    } else {
        print_string("Interrupts disabled (initial).\n");
    }

    // Set up interrupt handling BEFORE enabling interrupts
    idt_init();
    
    pic_remap();
    
    enable_keyboard();

    __asm__ volatile("sti");

    // Verify interrupts are enabled
    __asm__ volatile("pushfq; pop %0" : "=r"(flags));
    if (flags & (1 << 9)) {
        print_string("Interrupts enabled successfully.\n");
    } else {
        print_string("Failed to enable interrupts!\n");
    }

    print_string("System ready! Type something...\n");

    // Test keyboard status
    itoa(inb(0x64), buffer, 16); // Reuse the same buffer
 

    // Test interrupt (optional - remove this line if you don't want to test)
    // print_string("Testing software interrupt...\n");
    // __asm__ volatile("int $0x21");
    

    // Main kernel loop
    while (1) {
        // Keep CPU active but allow interrupts
        __asm__ volatile("hlt");
    }
}