#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>

#define FS_SIZE (64 * 1024)        // 64 KB file system
#define MAX_FILES 32               // Max number of files
#define MAX_FILENAME 12            // Max filename length (including null)
#define BLOCK_SIZE 512             // Block size in bytes
#define MAX_BLOCKS (FS_SIZE / BLOCK_SIZE) // Number of blocks

typedef struct {
    char name[MAX_FILENAME];       // File name
    uint32_t size;                 // File size in bytes
    uint16_t first_block;          // Index of first block in FAT
} fs_entry;

typedef struct {
    uint32_t magic;                // Magic number (e.g., 0xCAFE)
    uint32_t total_size;           // Total size of file system
    uint32_t num_files;            // Number of files
    fs_entry files[MAX_FILES];     // File entries
} fs_superblock;

typedef struct {
    uint16_t next_block;           // Next block index or 0xFFFF for end
} fs_fat_entry;

void fs_init(void);
int fs_create_file(const char *name);
int fs_write_file(const char *name, const char *data, uint32_t size);
int fs_read_file(const char *name, char *buffer, uint32_t max_size);
void fs_list_files(void);

#endif