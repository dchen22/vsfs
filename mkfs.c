#include "mkfs.h"
#include <unistd.h>
#include <time.h>

#define VSFS_MAGIC 0x56534653 // "VSFS" in hex

int format_disk(const char *disk_name, size_t disk_size, size_t max_files) {
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
    size_t num_inodes, num_data_blocks, num_bitmap_blocks;
    if (calculate_layout(disk_size, max_files, &num_inodes, &num_data_blocks, &num_bitmap_blocks) < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Write superblock
    if (write_superblock(map, disk_size, max_files) < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Initialize inode table
    if (initialize_inode_table(map, num_inodes) < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Initialize data bitmap
    if (initialize_data_bitmap(map, num_data_blocks) < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Create root directory
    if (create_root_directory(map) < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Cleanup and return
    cleanup_disk(map, disk_size, fd);
    return 0;
}

int write_superblock(char *disk_map, size_t disk_size, size_t max_files) {
    // TODO: Implement superblock writing
    // - Create superblock_t structure
    // - Set magic number to VSFS_MAGIC
    // - Set block_size to BLOCK_SIZE
    // - Calculate and set total_blocks from disk_size
    // - Set inode_blocks, data_blocks, bitmap_blocks based on layout
    // - Set total_inodes and next_free_inode (start at 2, skip 0 and 1 for root)
    // - Initialize free_blocks counter
    // - Set root_inode to 1 (first inode after reserved inode 0)
    // - Write superblock to first block of disk_map
    // - Return 0 on success, -1 on error
    
    return 0;
}

int initialize_inode_table(char *disk_map, size_t num_inodes) {
    // TODO: Implement inode table initialization
    // - Calculate starting block for inode table (after superblock)
    // - Initialize all inodes to zero (already done by memset)
    // - Set inode 0 as reserved/unused (mode = 0)
    // - All inodes are free initially (mode = 0 means free)
    // - Set up inode 1 for root directory (will be done in create_root_directory)
    // - No inode bitmap needed - using fixed allocation with next_free_inode counter
    // - Return 0 on success, -1 on error
    
    return 0;
}

int initialize_data_bitmap(char *disk_map, size_t num_data_blocks) {
    // TODO: Implement data bitmap initialization
    // - Calculate starting block for data bitmap (after inode table)
    // - Initialize bitmap to all zeros (all blocks free)
    // - Mark block 0 as used (if needed for special purposes)
    // - Each bit represents one data block (0 = free, 1 = used)
    // - Return 0 on success, -1 on error

    
    return 0;
}

int create_root_directory(char *disk_map) {
    // TODO: Implement root directory creation
    // - Allocate first data block for root directory
    // - Set up inode 1 as directory inode
    // - Set mode to directory (S_IFDIR)
    // - Set size to BLOCK_SIZE
    // - Set nlinks to 2 (for . and .. entries)
    // - Set timestamps to current time
    // - Create . and .. directory entries
    // - Update data bitmap to mark allocated block as used
    // - Update superblock next_free_inode to 2 (since inode 1 is now used)
    // - Return 0 on success, -1 on error
    
    return 0;
}

int calculate_layout(size_t disk_size, size_t max_files, 
                    size_t *num_inodes, size_t *num_data_blocks, 
                    size_t *num_bitmap_blocks) {
    // TODO: Implement layout calculation
    // - Calculate total_blocks from disk_size / BLOCK_SIZE
    // - Reserve 1 block for superblock
    // - Calculate num_inodes based on max_files (with some overhead)
    // - Calculate inode_blocks = (num_inodes * INODE_SIZE) / BLOCK_SIZE
    // - Calculate num_data_blocks = total_blocks - 1 - inode_blocks - bitmap_blocks
    // - Calculate num_bitmap_blocks = (num_data_blocks + 7) / 8 / BLOCK_SIZE
    // - The bitmap is a bitmap of all blocks in the disk, including superblock and inode table
    // - Validate that layout fits within disk_size
    // - Set output parameters
    // - Return 0 on success, -1 on error
    size_t num_superblock_blocks = 1;
    size_t num_total_blocks = disk_size / BLOCK_SIZE;
    *num_bitmap_blocks = (num_total_blocks + 7) / 8 / BLOCK_SIZE;  
    *num_inodes = (max_files * INODE_SIZE) / BLOCK_SIZE;
    *num_data_blocks = num_total_blocks - num_superblock_blocks - *num_inodes - num_bitmap_blocks;

    assert(num_bitmap_blocks >= 2);
    assert(num_data_blocks >= 2);
    assert(num_total_blocks == num_superblock_blocks + num_bitmap_blocks + num_inodes + num_data_blocks);

    return 0;
}

void cleanup_disk(char *disk_map, size_t disk_size, int fd) {
    // TODO: Implement cleanup
    // - Unmap the memory-mapped file
    // - Close the file descriptor
    // - Handle any cleanup errors gracefully
    
    if (disk_map != MAP_FAILED) {
        munmap(disk_map, disk_size);
    }
    if (fd >= 0) {
        close(fd);
    }
}