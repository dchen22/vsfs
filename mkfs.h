#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

#define BLOCK_SIZE 4096 // Size of a block in bytes
#define MAX_FILENAME_LEN 255
#define MAX_INODES 1024
#define INODE_SIZE 64

// VSFS Superblock structure
typedef struct {
    uint32_t magic;           // Magic number to identify VSFS
    uint32_t disk_size;       // Size of the disk in bytes
    uint32_t block_size;      // Size of each block
    uint32_t num_total_blocks;    // Total number of blocks in the filesystem
    uint32_t num_inode_table_blocks;    // Number of blocks used for inodes
    uint32_t num_data_blocks;     // Number of blocks used for data
    uint32_t num_data_bitmap_blocks;   // Number of blocks used for data bitmap
    uint32_t num_inode_bitmap_blocks;  // Number of blocks used for inode bitmap
    uint32_t num_max_inodes;    // Maximum number of files in the filesystem
    uint32_t num_used_inodes; // Number of used inodes
    uint32_t num_free_blocks;     // Number of free data blocks
} superblock_t;

// VSFS Inode structure
typedef struct {
    uint32_t size;            // File size in bytes
    uint32_t atime;           // Access time
    uint32_t mtime;           // Modification time
    uint32_t ctime;           // Creation time
    uint32_t nlinks;          // Number of hard links
    uint32_t blocks[12];      // Direct block pointers (12 direct blocks)
    uint32_t indirect;        // Indirect block pointer
} inode_t;

// VSFS Directory entry structure
typedef struct {
    uint32_t inode;           // Inode number
    uint16_t rec_len;         // Record length
    uint8_t name_len;         // Name length
    uint8_t file_type;        // File type
    char name[MAX_FILENAME_LEN]; // File name
} dirent_t;

// Global filesystem pointers
extern superblock_t *sb;
extern char *inode_bitmap;
extern char *data_bitmap;
extern char *inode_table;
extern char *data_section;

// Function declarations for VSFS formatting
int format_disk(const char *disk_name, size_t disk_size, size_t max_files);
int write_superblock(char *disk_map, size_t disk_size, size_t max_files, 
                    size_t num_total_blocks, size_t num_inode_table_blocks, size_t num_data_blocks, 
                    size_t num_data_bitmap_blocks, size_t num_inode_bitmap_blocks);
int initialize_inode_table();
int initialize_data_bitmap();
int initialize_inode_bitmap();
int create_root_directory();
int calculate_layout(char *disk_map, size_t disk_size, size_t max_files, 
                    size_t *num_total_blocks, size_t *num_inode_table_blocks, size_t *num_data_blocks, 
                    size_t *num_data_bitmap_blocks, size_t *num_inode_bitmap_blocks);
void cleanup_disk(char *disk_map, size_t disk_size, int fd);