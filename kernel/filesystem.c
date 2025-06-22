#include "filesystem.h"
#include "vga.h"
#include "utils.h"
#include "task.h"
#include <string.h>

static uint8_t fs_buffer[FS_SIZE]; // 64 KB in-memory file system
static fs_superblock *superblock = (fs_superblock *)fs_buffer;
static fs_fat_entry *fat = (fs_fat_entry *)(fs_buffer + sizeof(fs_superblock));
static uint8_t *data_area = fs_buffer + sizeof(fs_superblock) + sizeof(fs_fat_entry) * MAX_BLOCKS;

void fs_init(void) {
    enter_critical_section();
    superblock->magic = 0xCAFE;
    superblock->total_size = FS_SIZE;
    superblock->num_files = 0;
    
    for (int i = 0; i < MAX_FILES; i++) {
        superblock->files[i].name[0] = '\0';
        superblock->files[i].size = 0;
        superblock->files[i].first_block = 0xFFFF;
    }
    
    for (int i = 0; i < MAX_BLOCKS; i++) {
        fat[i].next_block = 0xFFFF; // Free block
    }
    
    // Create a test file
    fs_create_file("welcome.txt");
    const char *welcome_data = "Welcome to CAPTAIN-OS!\n";
    fs_write_file("welcome.txt", welcome_data, strlen(welcome_data));
    
    exit_critical_section();
}

int fs_create_file(const char *name) {
    enter_critical_section();
    if (superblock->num_files >= MAX_FILES) {
        print_string("Error: Max files reached!\n");
        exit_critical_section();
        return -1;
    }
    
    // Check if file exists
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(superblock->files[i].name, name) == 0) {
            print_string("Error: File already exists!\n");
            exit_critical_section();
            return -1;
        }
    }
    
    // Find free file entry
    int file_index = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (superblock->files[i].name[0] == '\0') {
            file_index = i;
            break;
        }
    }
    
    if (file_index == -1) {
        print_string("Error: No free file entries!\n");
        exit_critical_section();
        return -1;
    }
    
    // Copy name (truncate if too long)
    int i = 0;
    while (name[i] && i < MAX_FILENAME - 1) {
        superblock->files[file_index].name[i] = name[i];
        i++;
    }
    superblock->files[file_index].name[i] = '\0';
    superblock->files[file_index].size = 0;
    superblock->files[file_index].first_block = 0xFFFF;
    superblock->num_files++;
    
    exit_critical_section();
    return 0;
}

int fs_write_file(const char *name, const char *data, uint32_t size) {
    enter_critical_section();
    int file_index = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(superblock->files[i].name, name) == 0) {
            file_index = i;
            break;
        }
    }
    
    if (file_index == -1) {
        print_string("Error: File not found!\n");
        exit_critical_section();
        return -1;
    }
    
    // Free existing blocks
    uint16_t block = superblock->files[file_index].first_block;
    while (block != 0xFFFF) {
        uint16_t next = fat[block].next_block;
        fat[block].next_block = 0xFFFF;
        block = next;
    }
    
    // Allocate new blocks
    uint32_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint16_t first_block = 0xFFFF;
    uint16_t *last_block_ptr = &first_block;
    
    for (uint32_t i = 0; i < blocks_needed; i++) {
        int free_block = -1;
        for (int j = 0; j < MAX_BLOCKS; j++) {
            if (fat[j].next_block == 0xFFFF) {
                free_block = j;
                break;
            }
        }
        if (free_block == -1) {
            print_string("Error: Out of disk space!\n");
            exit_critical_section();
            return -1;
        }
        *last_block_ptr = free_block;
        fat[free_block].next_block = 0xFFFF;
        last_block_ptr = &fat[free_block].next_block;
    }
    
    // Write data
    block = first_block;
    uint32_t bytes_written = 0;
    while (block != 0xFFFF && bytes_written < size) {
        uint32_t to_write = size - bytes_written > BLOCK_SIZE ? BLOCK_SIZE : size - bytes_written;
        for (uint32_t i = 0; i < to_write; i++) {
            data_area[block * BLOCK_SIZE + i] = data[bytes_written + i];
        }
        bytes_written += to_write;
        block = fat[block].next_block;
    }
    
    superblock->files[file_index].first_block = first_block;
    superblock->files[file_index].size = size;
    
    exit_critical_section();
    return 0;
}

int fs_read_file(const char *name, char *buffer, uint32_t max_size) {
    enter_critical_section();
    int file_index = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(superblock->files[i].name, name) == 0) {
            file_index = i;
            break;
        }
    }
    
    if (file_index == -1) {
        print_string("Error: File not found!\n");
        exit_critical_section();
        return -1;
    }
    
    uint32_t size = superblock->files[file_index].size;
    if (size > max_size) {
        print_string("Error: Buffer too small!\n");
        exit_critical_section();
        return -1;
    }
    
    uint16_t block = superblock->files[file_index].first_block;
    uint32_t bytes_read = 0;
    while (block != 0xFFFF && bytes_read < size) {
        uint32_t to_read = size - bytes_read > BLOCK_SIZE ? BLOCK_SIZE : size - bytes_read;
        for (uint32_t i = 0; i < to_read; i++) {
            buffer[bytes_read + i] = data_area[block * BLOCK_SIZE + i];
        }
        bytes_read += to_read;
        block = fat[block].next_block;
    }
    
    buffer[bytes_read] = '\0'; // Null-terminate for printing
    exit_critical_section();
    return bytes_read;
}

void fs_list_files(void) {
    enter_critical_section();
    print_string("Files in system:\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (superblock->files[i].name[0] != '\0') {
            print_string("  ");
            print_string(superblock->files[i].name);
            print_string(" (");
            char buffer[16];
            itoa(superblock->files[i].size, buffer, 10);
            print_string(buffer);
            print_string(" bytes)\n");
        }
    }
    exit_critical_section();
}