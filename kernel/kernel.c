#include "idt.h"
#include "vga.h"
#include "utils.h"
#include "pit.h"
#include "task.h"

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
    } else if (input_buffer[0] == 'a' && input_buffer[1] == 'n' && input_buffer[2] == 'i' && input_buffer[3] == 'm' && input_buffer[4] == 'e' && input_buffer[5] == '\0') {
        print_string("  .--.   .--.\n");
        print_string(" (o  o) (o  o)\n");
        print_string("  |  |   |  |\n");
        print_string("  '--'   '--'\n");
    } else if (input_buffer[0] == 't' && input_buffer[1] == 'a' && input_buffer[2] == 's' && input_buffer[3] == 'k' && input_buffer[4] == 's' && input_buffer[5] == '\0') {
        print_string("Running tasks:\n");
        print_string("Task 0: Shell (Interactive)\n");
        print_string("Task 1: Background Worker\n");
        print_string("Task 2: Idle Task\n");
        char buffer[16];
        print_string("Background task counter: ");
        itoa(background_counter, buffer, 10);
        print_string(buffer);
        print_string("\n");
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
        print_string("Commands: clear, echo <text>, help, os, anime, tasks\n");
        print_string("The background task is running concurrently.\n");
        print_string("\n> ");
        shell_initialized = 1;
        exit_critical_section();
    }
    
    while (1) {
        // Shell task just yields and waits for keyboard input
        task_yield();
        for (volatile int i = 0; i < 100000; i++);  // Longer delay to reduce CPU usage
    }
}

void background_task(void) {
    while (1) {
        background_counter++;
        
        // Only update display every 10000 iterations to reduce noise
        if (background_counter % 10000 == 0) {
            enter_critical_section();
            
            // Save current cursor position
            uint8_t saved_row = cursor_row;
            uint8_t saved_col = cursor_col;
            
            // Display background task status in top right corner
            cursor_row = 0;
            cursor_col = 60;
            print_string("[BG:");
            char buffer[8];
            itoa(background_counter / 1000, buffer, 10);
            print_string(buffer);
            print_string("k]");
            
            // Restore cursor position
            cursor_row = saved_row;
            cursor_col = saved_col;
            
            exit_critical_section();
        }
        
        // Yield every 1000 iterations to be more cooperative
        if (background_counter % 1000 == 0) {
            task_yield();
        }
        
        // Small delay to prevent overwhelming the system
        for (volatile int i = 0; i < 1000; i++);
    }
}


void kernel_main(void) {
    __asm__ volatile("cli");

    idt_init();
    pic_remap();
    pit_init(100);  // 100Hz timer

    enable_keyboard();
    task_init();

    task_create(shell_task);
    task_create(background_task);
   
    for (volatile int i = 0; i < 1000000; i++);
    
    __asm__ volatile("sti");

    start_multitasking();

    print_string("ERROR: Multitasking failed to start!\n");
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}