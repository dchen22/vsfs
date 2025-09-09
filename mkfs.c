#define _POSIX_C_SOURCE 200809L
#include "mkfs.h"
#include "helpers.h"
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <sys/stat.h>

#define VSFS_MAGIC 0x56534653 // "VSFS" in hex

// Global filesystem pointers
superblock_t *sb = NULL;
char *inode_bitmap = NULL;
char *data_bitmap = NULL;
char *inode_table = NULL;
char *data_section = NULL;

int format_disk(const char *disk_name, size_t disk_size, size_t max_files) {
    assert(sizeof(superblock_t) <= BLOCK_SIZE);   // superblock needs to fit in a block

    if (disk_size < 2 * BLOCK_SIZE) {   // superblock and at least 1 bitmap block
        fprintf(stderr, "Disk size is too small\n");
        return -1;
    }

    if (max_files == 0) {
        fprintf(stderr, "Max files is 0\n");
        return -1;
    }

    int fd = open(disk_name, O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        perror("Failed to open disk");
        return -1; // Return -1 on error
    }

    // Ensure the file is at least N bytes
    if (ftruncate(fd, disk_size) == -1) {
        perror("ftruncate");
        close(fd);
        return -1;
    }

     // Map the file into memory
    char *map = mmap(NULL, disk_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }

    memset(map, 0, disk_size); // Initialize the mapped memory to zero

    // Calculate layout for the filesystem
    size_t num_total_blocks, num_inode_table_blocks, num_data_blocks, num_inode_bitmap_blocks, num_data_bitmap_blocks;
    if (calculate_layout(map, disk_size, max_files, &num_total_blocks, &num_inode_table_blocks, &num_data_blocks, &num_data_bitmap_blocks, &num_inode_bitmap_blocks) < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Write superblock
    if (write_superblock(map, disk_size, max_files, num_total_blocks, num_inode_table_blocks, num_data_blocks, num_inode_bitmap_blocks, num_data_bitmap_blocks) < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Initialize inode table
    if (initialize_inode_table() < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Initialize inode bitmap
    if (initialize_inode_bitmap() < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Initialize data bitmap
    if (initialize_data_bitmap() < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Create root directory
    if (create_root_directory() < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Cleanup and return
    // cleanup_disk(map, disk_size, fd);
    return 0;
}

int write_superblock(char *disk_map, size_t disk_size, size_t max_files, 
    size_t num_total_blocks, size_t num_inode_table_blocks, size_t num_data_blocks, 
    size_t num_inode_bitmap_blocks, size_t num_data_bitmap_blocks) {
    // TODO: Implement superblock writing
    // - Create superblock_t structure
    // - Set magic number to VSFS_MAGIC
    // - Set block_size to BLOCK_SIZE
    // - Calculate and set total_blocks from disk_size
    // - Set inode_blocks, data_blocks, bitmap_blocks based on layout
    // - Set total_inodes and next_free_inode (start at 1, since 0 is root)
    // - Initialize free_blocks counter
    // - Set root_inode to 0 
    // - Write superblock to first block of disk_map
    // - Return 0 on success, -1 on error

    // Set superblock pointer to point to the first block of disk_map
    sb = (superblock_t *)disk_map;
    
    // Initialize superblock fields
    sb->magic = VSFS_MAGIC;
    sb->disk_size = disk_size;
    sb->block_size = BLOCK_SIZE;
    sb->num_total_blocks = num_total_blocks;
    sb->num_inode_table_blocks = num_inode_table_blocks;
    sb->num_data_blocks = num_data_blocks;
    sb->num_data_bitmap_blocks = num_data_bitmap_blocks;
    sb->num_inode_bitmap_blocks = num_inode_bitmap_blocks;
    sb->num_max_inodes = max_files;
    sb->num_used_inodes = 0; 
    sb->num_free_blocks = num_data_blocks;
    
    return 0;
}

int initialize_inode_table() {
    // TODO: Implement inode table initialization
    // - Calculate starting block for inode table (after superblock)
    // - Initialize all inodes to zero (already done by memset)
    // - All inodes are free initially (mode = 0 means free)
    // - Set up inode 0 for root directory (will be done in create_root_directory)
    // - Inode bitmap is now used for proper inode allocation/deallocation
    // - Return 0 on success, -1 on error

    memset(inode_table, 0, sb->num_inode_table_blocks * BLOCK_SIZE);
    
    return 0;
}

int initialize_data_bitmap() {
    // TODO: Implement data bitmap initialization
    // - Calculate starting block for data bitmap (after inode bitmap)
    // - Initialize bitmap to all zeros (all blocks free)
    // - Mark block 0 as used (if needed for special purposes)
    // - Each bit represents one data block (0 = free, 1 = used)
    // - Return 0 on success, -1 on error

    // Clear the entire data bitmap (set all bits to 0)
    if (bitmapclear(data_bitmap, sb->num_total_blocks) < 0) {
        return -1;
    }
    
    // Mark block 0 (superblock) and both bitmaps' blocks as used
    for (size_t i = 0; i < 1 + sb->num_inode_bitmap_blocks + sb->num_data_bitmap_blocks; i++) {
        if (bitmapset(data_bitmap, sb->num_total_blocks, i, true) < 0) {
            return -1;
        }
    }

    return 0;
}

int initialize_inode_bitmap() {
    // TODO: Implement inode bitmap initialization
    // - Initialize inode bitmap to all zeros (all inodes free)
    // - Each bit represents one inode (0 = free, 1 = used)
    // - Return 0 on success, -1 on error

    // Clear the entire inode bitmap (set all bits to 0)
    if (bitmapclear(inode_bitmap, sb->num_max_inodes) < 0) {
        return -1;
    }
    

    return 0;
}

int create_root_directory() {
    // TODO: Implement root directory creation
    // - Allocate first data block for root directory
    // - Set up inode 0 as directory inode
    // - Set mode to directory (S_IFDIR)
    // - Set size to BLOCK_SIZE
    // - Set nlinks to 2 (for . and .. entries)
    // - Set timestamps to current time
    // - Create . and .. directory entries
    // - Update data bitmap to mark allocated block as used
    // - Update superblock next_free_inode to 1 (since inode 0 is now used)
    // - Return 0 on success, -1 on error

    assert(sb->num_used_inodes < sb->num_max_inodes);

    inode_t root_inode;
    root_inode.size = 0;
    root_inode.atime = time(NULL);
    root_inode.mtime = time(NULL);
    root_inode.ctime = time(NULL);
    root_inode.nlinks = 2;
    root_inode.blocks[0] = 0;   // point at superblock to imply unused
    root_inode.indirect = 0;    // point at first inode to imply unused

    if (bitmapalloc(inode_bitmap, sb->num_max_inodes) != 0) {
        return -1;
    }
    memcpy(inode_table, &root_inode, sizeof(root_inode));
    sb->num_used_inodes++;

    // Mark inode 0 as used in the inode bitmap (already done in initialize_inode_bitmap)
    // No need to do it again here

    return 0;
}

int calculate_layout(char *disk_map, size_t disk_size, size_t max_files, 
                    size_t *num_total_blocks, size_t *num_inode_table_blocks, size_t *num_data_blocks, 
                    size_t *num_data_bitmap_blocks, size_t *num_inode_bitmap_blocks) {
    // TODO: Implement layout calculation
    // - Calculate total_blocks from disk_size / BLOCK_SIZE
    // - Reserve 1 block for superblock
    // - Calculate num_inodes based on max_files (with some overhead)
    // - Calculate inode_blocks = (num_inodes * INODE_SIZE) / BLOCK_SIZE
    // - Calculate num_inode_bitmap_blocks = ceildiv(max_inodes, BLOCK_SIZE * 8)
    // - Calculate num_data_bitmap_blocks = ceildiv(total_blocks, BLOCK_SIZE * 8)
    // - Calculate num_data_blocks = total_blocks - 1 - inode_blocks - inode_bitmap_blocks - data_bitmap_blocks
    // - The data bitmap tracks all blocks in the disk
    // - The inode bitmap tracks all inodes
    // - Validate that layout fits within disk_size
    // - Set output parameters
    // - Return 0 on success, -1 on error
    size_t num_superblock_blocks = 1;
    *num_total_blocks = disk_size / BLOCK_SIZE;
    *num_inode_bitmap_blocks = ceildiv(max_files, BLOCK_SIZE * 8);
    *num_data_bitmap_blocks = ceildiv(*num_total_blocks, BLOCK_SIZE * 8);
    *num_inode_table_blocks = (max_files * INODE_SIZE) / BLOCK_SIZE;
    *num_data_blocks = *num_total_blocks - num_superblock_blocks - *num_inode_table_blocks - *num_inode_bitmap_blocks - *num_data_bitmap_blocks;

    assert(*num_data_bitmap_blocks >= 1);    // superblock must exist, so at least 1 bitmap block to keep track of superblock
    assert(*num_inode_bitmap_blocks >= 1);   // at least 1 inode bitmap block
    assert(*num_total_blocks == num_superblock_blocks + *num_inode_bitmap_blocks + *num_data_bitmap_blocks + *num_inode_table_blocks + *num_data_blocks);

    inode_bitmap = disk_map + BLOCK_SIZE;
    data_bitmap = inode_bitmap + *num_inode_bitmap_blocks * BLOCK_SIZE;
    inode_table = data_bitmap + *num_data_bitmap_blocks * BLOCK_SIZE;
    data_section = inode_table + *num_inode_table_blocks * BLOCK_SIZE;

    return 0;
}


void cleanup_disk(char *disk_map, size_t disk_size, int fd) {
    // Unmap the memory-mapped file
    if (disk_map != MAP_FAILED) {
        if (munmap(disk_map, disk_size) == -1) {
            perror("munmap failed");
        }
    }
    
    // Close the file descriptor
    if (fd >= 0) {
        if (close(fd) == -1) {
            perror("close failed");
        }
    }
    
    // Clear the global filesystem pointers
    sb = NULL;
    inode_bitmap = NULL;
    data_bitmap = NULL;
    inode_table = NULL;
    data_section = NULL;
}