void kernel_main(void) {
    char *vga_buffer = (char *)0xB8000; // VGA text buffer (80x25 text mode)
    const char *msg = "Hello, CAPTAIN_SKR";
    for (int i = 0; msg[i] != '\0'; i++) {
        vga_buffer[i * 2] = msg[i];      // Character
        vga_buffer[i * 2 + 1] = 0x0F;    // Attribute: White text on black background
    }
    while (1); // Infinite loop to prevent kernel exit
}