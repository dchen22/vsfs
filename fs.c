#include "fs.h"
#include "mkfs.h"
#include "helpers.h"
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
    
   
    
    return 0;
}

int create_file(const char *filename) {
    if (sb->num_used_inodes >= sb->num_max_inodes) {
        printf("FAILURE: Reached max number of inodes\n");
        return -1;
    }
    // assume all files are stored in the root directory for now
    int new_inode_index = bitmapalloc(inode_bitmap, sb->num_max_inodes);
    if (new_inode_index < 0) {
        printf("Failed to allocate new inode\n");
        return -1;
    }
    inode_t* new_inode = (inode_t*)(inode_table + new_inode_index * sizeof(inode_t));
    strncpy(new_inode->name, filename, MAX_FILENAME_LEN);
    new_inode->name[MAX_FILENAME_LEN-1] = '\0';   // ensure null termination

    new_inode->size = 0;
    new_inode->is_directory = false;
    new_inode->is_allocated = true;
    new_inode->nlinks = 1; // root directory
    for (unsigned int i = 0; i < 12; i++) {
        new_inode->blocks[i] = 0;    // points at 0th disk block (superblock), not 0th data block
    }
    new_inode->indirect = 0;
    sb->num_used_inodes++;

    return 0;
}

int print_all_files(void) {
    for (unsigned int i = 0; i < sb->num_max_inodes; i++) {
        if (bitmapget(inode_bitmap, sb->num_max_inodes, i) == 1) {
            inode_t *inode = (inode_t*)(inode_table + i * sizeof(inode_t));
            printf("%s\n", inode->name);
        }
    }
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


void print_fs_status(void) {
    printf("Filesystem loaded successfully. File system initial details:\n");
    printf("  Disk name: %s\n", sb->disk_name);
    printf("  Disk size: %u bytes\n", sb->disk_size);
    printf("  Block size: %u\n", sb->block_size);
    printf("  Total blocks: %u blocks\n", sb->num_total_blocks);
    printf("  Inode size: %lu\n", sizeof(inode_t));
    printf("  Max files: %u\n", sb->num_max_inodes);
    printf("  Reserved blocks: %u\n", 1 + sb->num_inode_bitmap_blocks + sb->num_data_bitmap_blocks + sb->num_inode_table_blocks + sb->num_used_data_blocks);
    printf("    Breakdown: \n");
    printf("      Superblock: 1\n");
    printf("      Inode bitmap: %u\n", sb->num_inode_bitmap_blocks);
    printf("      Data bitmap: %u\n", sb->num_data_bitmap_blocks);
    printf("      Inode table: %u\n", sb->num_inode_table_blocks);
    printf("      Data section: %u\n", sb->num_used_data_blocks);
}