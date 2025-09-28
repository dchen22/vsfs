#include "fs.h"
#include "mkfs.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define VSFS_MAGIC 0x56534653 // "VSFS" in hex

// Global variables to track the loaded filesystem
static char *disk_map = NULL;
static int disk_fd = -1;
static size_t disk_size = 0;

int load_fs(const char *disk_name) {
    // Check if filesystem is already loaded
    if (disk_map != NULL) {
        fprintf(stderr, "Filesystem already loaded\n");
        return -1;
    }
    
    // Open the disk file
    disk_fd = open(disk_name, O_RDWR);
    if (disk_fd < 0) {
        fprintf(stderr, "Failed to open disk file '%s': %s\n", disk_name, strerror(errno));
        return -1;
    }
    
    // Get file size
    struct stat st;
    if (fstat(disk_fd, &st) < 0) {
        fprintf(stderr, "Failed to get file size: %s\n", strerror(errno));
        close(disk_fd);
        disk_fd = -1;
        return -1;
    }
    
    disk_size = st.st_size;
    
    // Memory map the file
    disk_map = mmap(NULL, disk_size, PROT_READ | PROT_WRITE, MAP_SHARED, disk_fd, 0);
    if (disk_map == MAP_FAILED) {
        fprintf(stderr, "Failed to memory map disk file: %s\n", strerror(errno));
        close(disk_fd);
        disk_fd = -1;
        return -1;
    }
    
    // Set up global pointers
    sb = (superblock_t *)disk_map;
    
    // Validate that this is a VSFS filesystem
    if (sb->magic != VSFS_MAGIC) {
        fprintf(stderr, "Invalid magic number: 0x%08x (expected 0x%08x)\n", 
                sb->magic, VSFS_MAGIC);
        unload_fs();
        return -1;
    }
    
    if (sb->block_size != BLOCK_SIZE) {
        fprintf(stderr, "Invalid block size: %u (expected %d)\n", 
                sb->block_size, BLOCK_SIZE);
        unload_fs();
        return -1;
    }
    
    // Calculate layout and set up pointers based on superblock information
    size_t offset = BLOCK_SIZE;  // Start after superblock
    
    // Set inode bitmap pointer
    inode_bitmap = disk_map + offset;
    offset += sb->num_inode_bitmap_blocks * BLOCK_SIZE;
    
    // Set data bitmap pointer
    data_bitmap = disk_map + offset;
    offset += sb->num_data_bitmap_blocks * BLOCK_SIZE;
    
    // Set inode table pointer
    inode_table = disk_map + offset;
    offset += sb->num_inode_table_blocks * BLOCK_SIZE;
    
    // Set data section pointer
    data_section = disk_map + offset;
    
    printf("Filesystem loaded successfully:\n");
    printf("  Disk: %s (%zu bytes, %u blocks)\n", disk_name, disk_size, sb->num_total_blocks);
    printf("  Max files: %u, Used inodes: %u, Free blocks: %u\n", 
           sb->num_max_inodes, sb->num_used_inodes, sb->num_free_blocks);
    
    return 0;
}

void unload_fs(void) {
    // Unmap memory
    if (disk_map != NULL && disk_map != MAP_FAILED) {
        munmap(disk_map, disk_size);
        disk_map = NULL;
    }
    
    // Close file
    if (disk_fd >= 0) {
        close(disk_fd);
        disk_fd = -1;
    }
    
    // Clear global pointers
    sb = NULL;
    inode_bitmap = NULL;
    data_bitmap = NULL;
    inode_table = NULL;
    data_section = NULL;
    
    disk_size = 0;
}
