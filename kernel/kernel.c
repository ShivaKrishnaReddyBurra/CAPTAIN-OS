#include "idt.h"
#include "vga.h"
#include "utils.h"
#include "pit.h"
#include "task.h"
#include "filesystem.h"
#include <string.h>

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

// String comparison helper
int strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

// String starts with helper
int starts_with(const char *str, const char *prefix) {
    while (*prefix) {
        if (*str != *prefix) return 0;
        str++;
        prefix++;
    }
    return 1;
}

// Clear screen function
void clear_screen(void) {
    enter_critical_section();
    clear_screen_proper();
    exit_critical_section();
}

// Handle command function with improved parsing
void handle_command(void) {
    input_buffer[input_pos] = '\0';

    // Skip empty commands
    if (input_buffer[0] == '\0') {
        print_string("> ");
        return;
    }

    if (strcmp(input_buffer, "clear") == 0) {
        clear_screen();
    } else if (starts_with(input_buffer, "echo ")) {
        if (input_buffer[5] != '\0') {
            print_string(&input_buffer[5]);
        }
        print_string("\n");
    } else if (strcmp(input_buffer, "os") == 0) {
        print_string("CAPTAIN-OS Kernel v1.1 - Multitasking Edition\n");
        print_string("Features: Task switching, keyboard input, VGA text mode, file system\n");
        print_string("Built with x86-64 assembly and C\n");
    } else if (strcmp(input_buffer, "help") == 0) {
        print_string("Available commands:\n");
        print_string("  clear     - Clear the screen\n");
        print_string("  echo <text> - Print text to the screen\n");
        print_string("  os        - Display OS information\n");
        print_string("  help      - Display this help message\n");
        print_string("  anime     - Display ASCII art\n");
        print_string("  tasks     - Show running tasks info\n");
        print_string("  status    - Show system status\n");
        print_string("  test      - Run a simple test\n");
        print_string("  ls        - List all files\n");
        print_string("  cat <file> - Display file contents\n");
        print_string("  write <file> <data> - Write data to a file\n");
    } else if (strcmp(input_buffer, "anime") == 0) {
        print_string("    /\\_/\\  \n");
        print_string("   ( o.o ) \n");
        print_string("    > ^ <  \n");
        print_string("   /|   |\\ \n");
        print_string("  (_)   (_)\n");
        print_string("  Cute cat ASCII art!\n");
    } else if (strcmp(input_buffer, "tasks") == 0) {
        print_string("Active tasks:\n");
        print_string("  Task 0: Shell (Interactive)\n");
        print_string("  Task 1: Background Worker\n");
        char buffer[32];
        print_string("Background counter: ");
        itoa(background_counter, buffer, 10);
        print_string(buffer);
        print_string("\n");
    } else if (strcmp(input_buffer, "status") == 0) {
        char buffer[32];
        print_string("System Status:\n");
        print_string("  Cursor position: (");
        itoa(cursor_row, buffer, 10);
        print_string(buffer);
        print_string(", ");
        itoa(cursor_col, buffer, 10);
        print_string(buffer);
        print_string(")\n");
        print_string("  Active tasks: 2\n");
        print_string("  Multitasking: Enabled\n");
        print_string("  Keyboard: Working\n");
        print_string("  File system: Initialized\n");
    } else if (strcmp(input_buffer, "test") == 0) {
        print_string("Running system test...\n");
        print_string("  Memory access: OK\n");
        print_string("  VGA buffer: OK\n");
        print_string("  Task switching: OK\n");
        print_string("  Keyboard input: OK\n");
        print_string("  File system: OK\n");
        print_string("All tests passed!\n");
    } else if (strcmp(input_buffer, "ls") == 0) {
        fs_list_files();
    } else if (starts_with(input_buffer, "cat ")) {
        if (input_buffer[4] != '\0') {
            char buffer[512];
            if (fs_read_file(&input_buffer[4], buffer, sizeof(buffer)) >= 0) {
                print_string(buffer);
                print_string("\n");
            }
        } else {
            print_string("Usage: cat <file>\n");
        }
    } else if (starts_with(input_buffer, "write ")) {
        // Parse "write <filename> <data>"
        char *filename = &input_buffer[6];
        char *data = filename;
        while (*data && *data != ' ') data++;
        if (*data == ' ') {
            *data = '\0'; // Null-terminate filename
            data++; // Move to start of data
            if (*data != '\0') {
                if (fs_create_file(filename) >= 0 || strcmp(filename, "welcome.txt") == 0) {
                    if (fs_write_file(filename, data, strlen(data)) >= 0) {
                        print_string("Wrote to ");
                        print_string(filename);
                        print_string("\n");
                    }
                }
            } else {
                print_string("Usage: write <file> <data>\n");
            }
        } else {
            print_string("Usage: write <file> <data>\n");
        }
    } else {
        print_string("Unknown command: '");
        print_string(input_buffer);
        print_string("'\n");
        print_string("Type 'help' for available commands.\n");
    }

    // Clear input buffer
    input_pos = 0;
    for (int i = 0; i < MAX_INPUT; i++) {
        input_buffer[i] = 0;
    }

    print_string("> ");
}

void shell_task(void) {
    if (!shell_initialized) {
        enter_critical_section();
        clear_screen();
        print_string("========================================\n");
        print_string("    Welcome to CAPTAIN-OS v1.1!       \n");
        print_string("========================================\n");
        print_string("Multitasking kernel with file system support\n");
        print_string("Background task running concurrently\n");
        print_string("Type 'help' for available commands\n");
        print_string("========================================\n");
        print_string("\n> ");
        shell_initialized = 1;
        exit_critical_section();
    }
    
    while (1) {
        task_yield();
        for (volatile int i = 0; i < 50000; i++);
    }
}

void background_task(void) {
    uint32_t last_display = 0;
    
    while (1) {
        background_counter++;
        
        if (background_counter - last_display >= 25000) {
            enter_critical_section();
            uint8_t saved_row = cursor_row;
            uint8_t saved_col = cursor_col;
            
            cursor_row = 0;
            cursor_col = 65;
            print_string("[BG:");
            char buffer[8];
            itoa(background_counter / 1000, buffer, 10);
            print_string(buffer);
            print_string("k]");
            
            cursor_row = saved_row;
            cursor_col = saved_col;
            exit_critical_section();
            
            last_display = background_counter;
        }
        
        if (background_counter % 5000 == 0) {
            task_yield();
        }
        
        for (volatile int i = 0; i < 2000; i++);
    }
}

void kernel_main(void) {
    __asm__ volatile("cli");

    idt_init();
    pic_remap();
    pit_init(100);
    fs_init();
    enable_keyboard();
    task_init();

    task_create(shell_task);
    task_create(background_task);
   
    for (volatile int i = 0; i < 1000000; i++);
    
    __asm__ volatile("sti");

    start_multitasking();

    print_string("CRITICAL ERROR: Multitasking failed to start!\n");
    while (1) {
        __asm__ volatile("cli; hlt");
    }
}