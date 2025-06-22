#include "filesystem.h"
#include "vga.h"
#include "utils.h"
#include "task.h"
#include <string.h>

static uint8_t fs_buffer[FS_SIZE] __attribute__((aligned(16)));
static fs_superblock *superblock = (fs_superblock *)fs_buffer;
static fs_fat_entry *fat = (fs_fat_entry *)(fs_buffer + sizeof(fs_superblock));
static uint8_t *data_area = fs_buffer + sizeof(fs_superblock) + sizeof(fs_fat_entry) * MAX_BLOCKS;

static int fs_initialized = 0;

// Helper: String length with bounds checking
int fs_strlen(const char *str) {
    if (!str) return 0;
    int len = 0;
    while (str[len] && len < 4096) len++;  // Prevent infinite loop
    return len;
}

// Helper: String copy with bounds checking
void fs_strcpy(char *dest, const char *src) {
    if (!dest || !src) return;
    int i = 0;
    while (src[i] && i < MAX_FILENAME - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// Helper: String compare with null checks
int fs_strcmp(const char *str1, const char *str2) {
    if (!str1 || !str2) return -1;
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

// Helper: Memory set
void fs_memset(void *ptr, int value, uint32_t size) {
    if (!ptr) return;
    uint8_t *p = (uint8_t *)ptr;
    for (uint32_t i = 0; i < size; i++) {
        p[i] = value;
    }
}

// Helper: Extract filename from path
void extract_filename(const char *path, char *filename) {
    if (!path || !filename) return;
    
    const char *last_slash = path;
    const char *p = path;
    
    // Find last slash
    while (*p) {
        if (*p == '/') last_slash = p + 1;
        p++;
    }
    
    // Copy filename with bounds checking
    int i = 0;
    while (*last_slash && i < MAX_FILENAME - 1) {
        filename[i++] = *last_slash++;
    }
    filename[i] = '\0';
}

// Helper: Get parent path
void get_parent_path(const char *path, char *parent) {
    if (!path || !parent) return;
    
    int len = fs_strlen(path);
    int last_slash = -1;
    
    // Find last slash
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            last_slash = i;
            break;
        }
    }
    
    if (last_slash <= 0) {
        parent[0] = '/';
        parent[1] = '\0';
    } else {
        for (int i = 0; i < last_slash && i < 255; i++) {
            parent[i] = path[i];
        }
        parent[last_slash] = '\0';
    }
}

// Helper: Validate file index
int is_valid_file_index(int index) {
    return (index >= 0 && index < MAX_FILES);
}

// Helper: Validate block index
int is_valid_block_index(uint16_t block) {
    return (block > 0 && block < MAX_BLOCKS);
}

// Helper: Find file or directory by path
int find_entry(const char *path, int *parent_index) {
    if (!fs_initialized) return -1;
    if (parent_index) *parent_index = 255;
    
    // Handle root directory
    if (!path || path[0] == '\0' || (path[0] == '/' && path[1] == '\0')) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (superblock->files[i].name[0] != '\0' && 
                superblock->files[i].is_directory && 
                superblock->files[i].parent_index == 255) {
                return i;
            }
        }
        return -1;
    }
    
    // Start from root
    int current_index = find_entry("/", NULL);
    if (current_index == -1) return -1;
    
    char path_copy[256];
    fs_strcpy(path_copy, path);
    
    // Skip leading slash
    char *token = path_copy;
    if (*token == '/') token++;
    
    char *next_slash;
    while (*token) {
        // Find next component
        next_slash = token;
        while (*next_slash && *next_slash != '/') next_slash++;
        
        char component[MAX_FILENAME];
        int comp_len = next_slash - token;
        if (comp_len >= MAX_FILENAME) return -1;
        
        for (int i = 0; i < comp_len; i++) {
            component[i] = token[i];
        }
        component[comp_len] = '\0';
        
        // Search in current directory
        int found = -1;
        if (!is_valid_file_index(current_index) || !superblock->files[current_index].is_directory) {
            return -1;
        }
        
        uint16_t block = superblock->files[current_index].first_block;
        while (is_valid_block_index(block) && block != 0xFFFE) {
            uint8_t *block_data = data_area + block * BLOCK_SIZE;
            for (int i = 0; i < MAX_CHILDREN; i++) {
                uint8_t child_index = block_data[i];
                if (is_valid_file_index(child_index) &&
                    superblock->files[child_index].name[0] != '\0' &&
                    fs_strcmp(superblock->files[child_index].name, component) == 0) {
                    found = child_index;
                    break;
                }
            }
            if (found != -1) break;
            
            if (fat[block].next_block == 0xFFFE) break;
            block = fat[block].next_block;
        }
        
        if (found == -1) {
            if (parent_index) *parent_index = current_index;
            return -1;
        }
        
        if (parent_index) *parent_index = current_index;
        current_index = found;
        
        // Move to next component
        token = next_slash;
        if (*token == '/') token++;
    }
    
    return current_index;
}

// Helper: Allocate a block
uint16_t allocate_block(void) {
    if (!fs_initialized) return 0xFFFF;
    
    for (int i = 1; i < MAX_BLOCKS; i++) { // Start from 1, reserve 0
        if (fat[i].next_block == 0xFFFF) {
            fat[i].next_block = 0xFFFE; // Mark as used, end of chain
            return i;
        }
    }
    return 0xFFFF;
}

// Helper: Free blocks
void free_blocks(uint16_t first_block) {
    if (!fs_initialized || !is_valid_block_index(first_block)) return;
    
    uint16_t block = first_block;
    int safety_counter = 0;
    
    while (is_valid_block_index(block) && block != 0xFFFE && safety_counter < MAX_BLOCKS) {
        uint16_t next = fat[block].next_block;
        fat[block].next_block = 0xFFFF; // Mark as free
        block = next;
        safety_counter++;
    }
}

// Helper: Add child to directory
int add_child_to_directory(int parent_index, int child_index) {
    if (!fs_initialized || !is_valid_file_index(parent_index) || 
        !is_valid_file_index(child_index) || !superblock->files[parent_index].is_directory) {
        return -1;
    }
    
    uint16_t block = superblock->files[parent_index].first_block;
    
    // Try to find empty slot in existing blocks
    while (is_valid_block_index(block) && block != 0xFFFE) {
        uint8_t *block_data = data_area + block * BLOCK_SIZE;
        for (int i = 0; i < MAX_CHILDREN; i++) {
            if (block_data[i] == 255) {
                block_data[i] = child_index;
                return 0;
            }
        }
        
        if (fat[block].next_block == 0xFFFE) break;
        block = fat[block].next_block;
    }
    
    // Need new block
    uint16_t new_block = allocate_block();
    if (new_block == 0xFFFF) return -1;
    
    uint8_t *block_data = data_area + new_block * BLOCK_SIZE;
    fs_memset(block_data, 255, BLOCK_SIZE);
    block_data[0] = child_index;
    
    if (superblock->files[parent_index].first_block == 0xFFFF) {
        superblock->files[parent_index].first_block = new_block;
    } else if (is_valid_block_index(block)) {
        fat[block].next_block = new_block;
    }
    
    return 0;
}

// Helper: Remove child from directory
int remove_child_from_directory(int parent_index, int child_index) {
    if (!fs_initialized || !is_valid_file_index(parent_index) || 
        !is_valid_file_index(child_index) || !superblock->files[parent_index].is_directory) {
        return -1;
    }
    
    uint16_t block = superblock->files[parent_index].first_block;
    while (is_valid_block_index(block) && block != 0xFFFE) {
        uint8_t *block_data = data_area + block * BLOCK_SIZE;
        for (int i = 0; i < MAX_CHILDREN; i++) {
            if (block_data[i] == child_index) {
                block_data[i] = 255;
                return 0;
            }
        }
        if (fat[block].next_block == 0xFFFE) break;
        block = fat[block].next_block;
    }
    return -1;
}

void fs_init(void) {
    // Clear entire filesystem
    fs_memset(fs_buffer, 0, FS_SIZE);
    
    // Initialize superblock
    superblock->magic = 0xCAFE;
    superblock->total_size = FS_SIZE;
    superblock->num_files = 0;
    
    // Initialize file entries
    for (int i = 0; i < MAX_FILES; i++) {
        superblock->files[i].name[0] = '\0';
        superblock->files[i].size = 0;
        superblock->files[i].first_block = 0xFFFF;
        superblock->files[i].is_directory = 0;
        superblock->files[i].parent_index = 255;
    }
    
    // Initialize FAT - all blocks free
    for (int i = 0; i < MAX_BLOCKS; i++) {
        fat[i].next_block = 0xFFFF;
    }
    
    fs_initialized = 1;
    
    // Create root directory
    uint16_t root_block = allocate_block();
    if (root_block != 0xFFFF) {
        uint8_t *block_data = data_area + root_block * BLOCK_SIZE;
        fs_memset(block_data, 255, BLOCK_SIZE);
        
        fs_strcpy(superblock->files[0].name, "root");
        superblock->files[0].size = 0;
        superblock->files[0].first_block = root_block;
        superblock->files[0].is_directory = 1;
        superblock->files[0].parent_index = 255;
        superblock->num_files = 1;
    }
    
    // Create sample files safely
    if (fs_create_file("/welcome.txt") == 0) {
        fs_write_file("/welcome.txt", "Welcome to CAPTAIN-OS v1.3!\nImproved filesystem with better stability and error handling.", 97);
    }
    
    if (fs_create_file("/readme.txt") == 0) {
        fs_write_file("/readme.txt", "CAPTAIN-OS v1.3\nFeatures: Enhanced multitasking, robust filesystem, shell commands\nType 'help' for commands.", 106);
    }
    
    if (fs_create_directory("/docs") == 0) {
        if (fs_create_file("/docs/manual.txt") == 0) {
            fs_write_file("/docs/manual.txt", "CAPTAIN-OS Manual\n\nBasic Commands:\nls - list files\ncat - read file\nhelp - show help\nstatus - system info", 105);
        }
    }
}

int fs_create_file(const char *path) {
    if (!fs_initialized || !path) return -1;
    
    if (superblock->num_files >= MAX_FILES) {
        return -1;
    }
    
    // Check if file already exists
    int parent_index;
    if (find_entry(path, &parent_index) != -1) {
        return 0; // File exists, return success
    }
    
    // Get parent directory
    char parent_path[256];
    get_parent_path(path, parent_path);
    parent_index = find_entry(parent_path, NULL);
    
    if (parent_index == -1 || !superblock->files[parent_index].is_directory) {
        return -1;
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
        return -1;
    }
    
    // Extract filename
    char filename[MAX_FILENAME];
    extract_filename(path, filename);
    
    if (filename[0] == '\0') {
        return -1;
    }
    
    // Add to parent directory
    if (add_child_to_directory(parent_index, file_index) != 0) {
        return -1;
    }
    
    // Initialize file entry
    fs_strcpy(superblock->files[file_index].name, filename);
    superblock->files[file_index].size = 0;
    superblock->files[file_index].first_block = 0xFFFF;
    superblock->files[file_index].is_directory = 0;
    superblock->files[file_index].parent_index = parent_index;
    superblock->num_files++;
    
    return 0;
}

int fs_create_directory(const char *path) {
    if (!fs_initialized || !path) return -1;
    
    if (superblock->num_files >= MAX_FILES) {
        return -1;
    }
    
    // Check if directory already exists
    int parent_index;
    if (find_entry(path, &parent_index) != -1) {
        return 0; // Directory exists, return success
    }
    
    // Get parent directory
    char parent_path[256];
    get_parent_path(path, parent_path);
    parent_index = find_entry(parent_path, NULL);
    
    if (parent_index == -1 || !superblock->files[parent_index].is_directory) {
        return -1;
    }
    
    // Find free directory entry
    int dir_index = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (superblock->files[i].name[0] == '\0') {
            dir_index = i;
            break;
        }
    }
    
    if (dir_index == -1) {
        return -1;
    }
    
    // Allocate block for directory
    uint16_t block = allocate_block();
    if (block == 0xFFFF) {
        return -1;
    }
    
    uint8_t *block_data = data_area + block * BLOCK_SIZE;
    fs_memset(block_data, 255, BLOCK_SIZE);
    
    // Add to parent directory
    if (add_child_to_directory(parent_index, dir_index) != 0) {
        free_blocks(block);
        return -1;
    }
    
    // Extract directory name
    char dirname[MAX_FILENAME];
    extract_filename(path, dirname);
    
    // Initialize directory entry
    fs_strcpy(superblock->files[dir_index].name, dirname);
    superblock->files[dir_index].size = 0;
    superblock->files[dir_index].first_block = block;
    superblock->files[dir_index].is_directory = 1;
    superblock->files[dir_index].parent_index = parent_index;
    superblock->num_files++;
    
    return 0;
}

int fs_delete_file(const char *path) {
    if (!fs_initialized || !path) return -1;
    
    int parent_index;
    int file_index = find_entry(path, &parent_index);
    if (file_index == -1) {
        return -1;
    }
    
    // Don't allow deleting root
    if (superblock->files[file_index].parent_index == 255) {
        return -1;
    }
    
    fs_entry *entry = &superblock->files[file_index];
    
    // If directory, check if empty
    if (entry->is_directory) {
        uint16_t block = entry->first_block;
        while (is_valid_block_index(block) && block != 0xFFFE) {
            uint8_t *block_data = data_area + block * BLOCK_SIZE;
            for (int i = 0; i < MAX_CHILDREN; i++) {
                if (block_data[i] != 255) {
                    return -1; // Directory not empty
                }
            }
            if (fat[block].next_block == 0xFFFE) break;
            block = fat[block].next_block;
        }
    }
    
    // Remove from parent directory
    if (is_valid_file_index(parent_index)) {
        remove_child_from_directory(parent_index, file_index);
    }
    
    // Free blocks
    free_blocks(entry->first_block);
    
    // Clear entry
    entry->name[0] = '\0';
    entry->size = 0;
    entry->first_block = 0xFFFF;
    entry->is_directory = 0;
    entry->parent_index = 255;
    superblock->num_files--;
    
    return 0;
}

int fs_write_file(const char *path, const char *data, uint32_t size) {
    if (!fs_initialized || !path || !data) return -1;
    
    int parent_index;
    int file_index = find_entry(path, &parent_index);
    if (file_index == -1) {
        return -1;
    }
    
    if (superblock->files[file_index].is_directory) {
        return -1;
    }
    
    // Free existing blocks
    free_blocks(superblock->files[file_index].first_block);
    superblock->files[file_index].first_block = 0xFFFF;
    
    if (size == 0) {
        superblock->files[file_index].size = 0;
        return 0;
    }
    
    // Allocate blocks for data
    uint32_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint16_t first_block = 0xFFFF;
    uint16_t prev_block = 0xFFFF;
    
    for (uint32_t i = 0; i < blocks_needed; i++) {
        uint16_t block = allocate_block();
        if (block == 0xFFFF) {
            free_blocks(first_block);
            return -1;
        }
        
        if (first_block == 0xFFFF) {
            first_block = block;
        } else if (is_valid_block_index(prev_block)) {
            fat[prev_block].next_block = block;
        }
        prev_block = block;
    }
    
    // Write data to allocated blocks
    uint16_t block = first_block;
    uint32_t bytes_written = 0;
    
    while (is_valid_block_index(block) && bytes_written < size) {
        uint32_t to_write = (size - bytes_written > BLOCK_SIZE) ? BLOCK_SIZE : (size - bytes_written);
        uint8_t *block_data = data_area + block * BLOCK_SIZE;
        
        for (uint32_t i = 0; i < to_write; i++) {
            block_data[i] = data[bytes_written + i];
        }
        
        bytes_written += to_write;
        if (fat[block].next_block == 0xFFFE) break;
        block = fat[block].next_block;
    }
    
    superblock->files[file_index].first_block = first_block;
    superblock->files[file_index].size = size;
    
    return 0;
}

int fs_read_file(const char *path, char *buffer, uint32_t max_size) {
    if (!fs_initialized || !path || !buffer || max_size == 0) return -1;
    
    int parent_index;
    int file_index = find_entry(path, &parent_index);
    if (file_index == -1) {
        return -1;
    }
    
    if (superblock->files[file_index].is_directory) {
        return -1;
    }
    
    uint32_t size = superblock->files[file_index].size;
    if (size > max_size - 1) {
        size = max_size - 1;
    }
    
    uint16_t block = superblock->files[file_index].first_block;
    uint32_t bytes_read = 0;
    
    while (is_valid_block_index(block) && block != 0xFFFE && bytes_read < size) {
        uint32_t to_read = (size - bytes_read > BLOCK_SIZE) ? BLOCK_SIZE : (size - bytes_read);
        uint8_t *block_data = data_area + block * BLOCK_SIZE;
        
        for (uint32_t i = 0; i < to_read; i++) {
            buffer[bytes_read + i] = block_data[i];
        }
        
        bytes_read += to_read;
        if (fat[block].next_block == 0xFFFE) break;
        block = fat[block].next_block;
    }
    
    buffer[bytes_read] = '\0';
    return bytes_read;
}

void fs_list_files(const char *path) {
    if (!fs_initialized) {
        print_string("Error: Filesystem not initialized!\n");
        return;
    }
    
    int parent_index;
    int dir_index = find_entry(path, &parent_index);
    
    if (dir_index == -1 || !superblock->files[dir_index].is_directory) {
        print_string("Error: Directory not found!\n");
        return;
    }
    
    print_string("Directory listing for ");
    if (!path || path[0] == '\0' || (path[0] == '/' && path[1] == '\0')) {
        print_string("/");
    } else {
        print_string(path);
    }
    print_string(":\n");
    
    uint16_t block = superblock->files[dir_index].first_block;
    int file_count = 0;
    
    while (is_valid_block_index(block) && block != 0xFFFE) {
        uint8_t *block_data = data_area + block * BLOCK_SIZE;
        for (int i = 0; i < MAX_CHILDREN; i++) {
            uint8_t child_index = block_data[i];
            if (is_valid_file_index(child_index) &&
                superblock->files[child_index].name[0] != '\0') {
                
                fs_entry *child = &superblock->files[child_index];
                print_string("  ");
                print_string(child->name);
                
                if (child->is_directory) {
                    print_string("/");
                    print_string(" (DIR)");
                } else {
                    print_string(" (");
                    char buffer[16];
                    itoa(child->size, buffer, 10);
                    print_string(buffer);
                    print_string(" bytes)");
                }
                print_string("\n");
                file_count++;
            }
        }
        if (fat[block].next_block == 0xFFFE) break;
        block = fat[block].next_block;
    }
    
    if (file_count == 0) {
        print_string("  (empty)\n");
    }
    
    char buffer[16];
    print_string("Total: ");
    itoa(file_count, buffer, 10);
    print_string(buffer);
    print_string(" entries\n");
}

// Get filesystem statistics
void fs_get_stats(fs_stats *stats) {
    if (!fs_initialized || !stats) return;
    
    stats->total_files = 0;
    stats->total_directories = 0;
    stats->used_blocks = 0;
    stats->free_blocks = 0;
    stats->total_size = 0;
    
    // Count files and directories
    for (int i = 0; i < MAX_FILES; i++) {
        if (superblock->files[i].name[0] != '\0') {
            if (superblock->files[i].is_directory) {
                stats->total_directories++;
            } else {
                stats->total_files++;
                stats->total_size += superblock->files[i].size;
            }
        }
    }
    
    // Count used/free blocks
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (fat[i].next_block == 0xFFFF) {
            stats->free_blocks++;
        } else {
            stats->used_blocks++;
        }
    }
}