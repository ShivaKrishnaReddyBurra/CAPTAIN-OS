#include "idt.h"
#include "vga.h"
#include "utils.h"
#include "pit.h"
#include "task.h"
#include "framebuffer.h"

#define MAX_INPUT 80

// Shell state
char input_buffer[MAX_INPUT];
int input_pos = 0;
static int shell_initialized = 0;
static uint32_t background_counter = 0;

// Function declarations
void task_yield(void);
void task_sleep(uint32_t ticks);
void start_multitasking(void);

// Clear screen function
void clear_screen(void) {
    enter_critical_section();
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        ((uint16_t *)0xB8000)[i] = (uint16_t)(' ' | (0x0F << 8));
    }
    cursor_row = 0;
    cursor_col = 0;
    exit_critical_section();
}

// Handle command function
void handle_command(void) {
    input_buffer[input_pos] = '\0';

    if (input_buffer[0] == 'c' && input_buffer[1] == 'l' && input_buffer[2] == 'e' && input_buffer[3] == 'a' && input_buffer[4] == 'r' && input_buffer[5] == '\0') {
        clear_screen();
    } else if (input_buffer[0] == 'e' && input_buffer[1] == 'c' && input_buffer[2] == 'h' && input_buffer[3] == 'o' && input_buffer[4] == ' ') {
        print_string(&input_buffer[5]);
        print_string("\n");
    } else if (input_buffer[0] == 'o' && input_buffer[1] == 's' && input_buffer[2] == '\0') {
        print_string("CAPTAIN-OS Kernel v1.0 - Multitasking Edition\n");
    } else if (input_buffer[0] == 'h' && input_buffer[1] == 'e' && input_buffer[2] == 'l' && input_buffer[3] == 'p' && input_buffer[4] == '\0') {
        print_string("Available commands:\n");
        print_string("clear - Clear the screen\n");
        print_string("echo <text> - Print text to the screen\n");
        print_string("os - Display OS information\n");
        print_string("help - Display this help message\n");
        print_string("anime - Display an ASCII art\n");
        print_string("tasks - Show running tasks info\n");
        print_string("fbtest - Test framebuffer drawing\n");
        print_string("fbclear - Clear framebuffer\n");
    } else if (input_buffer[0] == 'a' && input_buffer[1] == 'n' && input_buffer[2] == 'i' && input_buffer[3] == 'm' && input_buffer[4] == 'e' && input_buffer[5] == '\0') {
        print_string("  .--.   .--.\n");
        print_string(" (o  o) (o  o)\n");
        print_string("  |  |   |  |\n");
        print_string("  '--'   '--'\n");
    } else if (input_buffer[0] == 't' && input_buffer[1] == 'a' && input_buffer[2] == 's' && input_buffer[3] == 'k' && input_buffer[4] == 's' && input_buffer[5] == '\0') {
        print_string("Running tasks:\n");
        print_string("Task 0: Shell (Interactive)\n");
        print_string("Task 1: Background Worker\n");
        print_string("Task 2: Graphics Task\n");
        char buffer[16];
        print_string("Background task counter: ");
        itoa(background_counter, buffer, 10);
        print_string(buffer);
        print_string("\n");
    } else if (input_buffer[0] == 'f' && input_buffer[1] == 'b' && input_buffer[2] == 't' && input_buffer[3] == 'e' && input_buffer[4] == 's' && input_buffer[5] == 't' && input_buffer[6] == '\0') {
        print_string("Testing framebuffer drawing...\n");
        // Draw test pattern
        for (uint32_t x = 0; x < 100 && x < fb_info.width; x++) {
            for (uint32_t y = 0; y < 100 && y < fb_info.height; y++) {
                uint32_t color = (x * 255 / 100) << 16 | (y * 255 / 100) << 8 | 0xFF;
                fb_draw_pixel(x, y, color);
            }
        }
        print_string("Test pattern drawn!\n");
    } else if (input_buffer[0] == 'f' && input_buffer[1] == 'b' && input_buffer[2] == 'c' && input_buffer[3] == 'l' && input_buffer[4] == 'e' && input_buffer[5] == 'a' && input_buffer[6] == 'r' && input_buffer[7] == '\0') {
        print_string("Clearing framebuffer...\n");
        fb_clear(0x000000);  // Black
        print_string("Framebuffer cleared!\n");
    } else if (input_buffer[0] != '\0') {
        print_string("Unknown command: ");
        print_string(input_buffer);
        print_string("\n");
    }

    // Clear input buffer
    input_pos = 0;
    for (int i = 0; i < MAX_INPUT; i++) {
        input_buffer[i] = 0;
    }

    print_string("> ");
}

void shell_task() {
    if (!shell_initialized) {
        enter_critical_section();
        clear_screen();
        print_string("Welcome to CAPTAIN-OS - Multitasking Edition!\n");
        print_string("Commands: clear, echo <text>, help, os, anime, tasks, fbtest, fbclear\n");
        print_string("The background and graphics tasks are running concurrently.\n");
        print_string("\n> ");
        shell_initialized = 1;
        exit_critical_section();
    }
    
    while (1) {
        task_yield();
        for (volatile int i = 0; i < 100000; i++);  // Longer delay to reduce CPU usage
    }
}

void background_task(void) {
    while (1) {
        background_counter++;
        
        if (background_counter % 10000 == 0) {
            enter_critical_section();
            uint8_t saved_row = cursor_row;
            uint8_t saved_col = cursor_col;
            cursor_row = 0;
            cursor_col = 60;
            print_string("[BG:");
            char buffer[8];
            itoa(background_counter / 1000, buffer, 10);
            print_string(buffer);
            print_string("k]");
            cursor_row = saved_row;
            cursor_col = saved_col;
            exit_critical_section();
        }
        
        if (background_counter % 1000 == 0) {
            task_yield();
        }
        
        for (volatile int i = 0; i < 1000; i++);
    }
}

void graphics_task(void) {
    static uint32_t frame_counter = 0;
    
    while (1) {
        // Only draw if framebuffer is available
        if (fb_info.addr != 0 && fb_info.width > 0 && fb_info.height > 0) {
            // Draw an animated square that changes color
            uint32_t start_x = (fb_info.width / 2) - 25;
            uint32_t start_y = (fb_info.height / 2) - 25;
            
            // Calculate color based on frame counter for animation
            uint8_t red = (frame_counter % 256);
            uint8_t green = ((frame_counter * 2) % 256);
            uint8_t blue = ((frame_counter * 3) % 256);
            uint32_t color = (red << 16) | (green << 8) | blue;

            for (uint32_t y = 0; y < 50 && (start_y + y) < fb_info.height; y++) {
                for (uint32_t x = 0; x < 50 && (start_x + x) < fb_info.width; x++) {
                    fb_draw_pixel(start_x + x, start_y + y, color);
                }
            }
            
            frame_counter++;
        }
        
        task_sleep(5);  // Update every 50ms for smooth animation
    }
}

void kernel_main(void) {
    __asm__ volatile("cli");

    // Get Multiboot information properly
    // The multiboot info structure is passed in %ebx register by GRUB
    void *multiboot_info_ptr;
    __asm__ volatile("mov %%rbx, %0" : "=r"(multiboot_info_ptr));

    print_string("CAPTAIN-OS starting...\n");
    
    idt_init();
    pic_remap();
    pit_init(100);  // 100Hz timer
    
    // Initialize framebuffer with proper multiboot info
    fb_init(multiboot_info_ptr);

    // Initialize framebuffer with a black screen if available
    if (fb_info.addr != 0) {
        fb_clear(0x000000);  // Clear to black
        print_string("Framebuffer initialized and cleared.\n");
    } else {
        print_string("Running in text mode only.\n");
    }

    // Initialize task system
    task_init();

    // Create all tasks
    task_create(shell_task);
    task_create(background_task);
    
    // Only create graphics task if framebuffer is available
    if (fb_info.addr != 0) {
        task_create(graphics_task);
    }
   
    // Small delay before starting multitasking
    for (volatile int i = 0; i < 1000000; i++);
    
    __asm__ volatile("sti");

    start_multitasking();

    print_string("ERROR: Multitasking failed to start!\n");
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}