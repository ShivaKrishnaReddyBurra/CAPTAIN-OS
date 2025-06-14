#include <stdint.h>

void kernel_main(void) {
    // Clear the screen first
    volatile uint16_t *vga_buffer = (uint16_t *)0xB8000;
    
    // Clear screen (80x25 = 2000 characters)
    for (int i = 0; i < 2000; i++) {
        vga_buffer[i] = (uint16_t)(' ' | (0x0F << 8)); // Space with white on black
    }
    
    // Display message
    const char *msg = "Hello, Captain_SKR - 64-bit OS Booted Successfully!";
    for (int i = 0; msg[i] != '\0'; i++) {
        vga_buffer[i] = (uint16_t)(msg[i] | (0x0F << 8)); // Character + Attribute (white on black)
    }
    
    // Display some additional info on the second line
    const char *info = "Running in 64-bit Long Mode";
    for (int i = 0; info[i] != '\0'; i++) {
        vga_buffer[80 + i] = (uint16_t)(info[i] | (0x0A << 8)); // Green on black
    }
    
    // Infinite loop to prevent kernel exit
    while (1) {
        __asm__ volatile ("hlt"); // Halt until next interrupt
    }
}