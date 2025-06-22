#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>

#define FS_SIZE (64 * 1024)        // 64 KB file system
#define MAX_FILES 32               // Max number of files
#define MAX_FILENAME 12            // Max filename length (including null)
#define BLOCK_SIZE 512             // Block size in bytes
#define MAX_BLOCKS (FS_SIZE / BLOCK_SIZE) // Number of blocks
#define MAX_CHILDREN 8             // Max files per directory

typedef struct {
    char name[MAX_FILENAME];       // File or directory name
    uint32_t size;                 // File size in bytes (0 for directories)
    uint16_t first_block;          // Index of first block in FAT (data or children)
    uint8_t is_directory;          // 1 for directory, 0 for file
    uint8_t parent_index;          // Index of parent directory (255 for root)
} fs_entry;

typedef struct {
    uint32_t magic;                // Magic number (e.g., 0xCAFE)
    uint32_t total_size;           // Total size of file system
    uint32_t num_files;            // Number of files
    fs_entry files[MAX_FILES];     // File and directory entries
} fs_superblock;

typedef struct {
    uint16_t next_block;           // Next block index or 0xFFFF for end
} fs_fat_entry;

typedef struct {
    uint32_t total_files;          // Number of files
    uint32_t total_directories;    // Number of directories
    uint32_t used_blocks;          // Number of used blocks
    uint32_t free_blocks;          // Number of free blocks
    uint32_t total_size;           // Total size of all files
} fs_stats;

void fs_init(void);
int fs_create_file(const char *path);
int fs_create_directory(const char *path);
int fs_delete_file(const char *path);
int fs_write_file(const char *path, const char *data, uint32_t size);
int fs_read_file(const char *path, char *buffer, uint32_t max_size);
void fs_list_files(const char *path);
int find_entry(const char *path, int *parent_index);
uint16_t allocate_block(void);
void free_blocks(uint16_t first_block);
void fs_get_stats(fs_stats *stats);

#endif