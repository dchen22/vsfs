#define _POSIX_C_SOURCE 200809L
#include "mkfs.h"
#include "helpers.h"
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <sys/stat.h>

#define VSFS_MAGIC 0x56534653 // "VSFS" in hex

fs_ctx_t fs_ctx;

int format_disk(const char *disk_name, size_t disk_size, size_t max_files) {
    if (disk_size < 2 * BLOCK_SIZE) {   // superblock and at least 1 bitmap block
        fprintf(stderr, "Disk size is too small\n");
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
    size_t num_total_blocks, num_inodes, num_data_blocks, num_bitmap_blocks;
    if (calculate_layout(map, disk_size, max_files, &num_total_blocks, &num_inodes, &num_data_blocks, &num_bitmap_blocks) < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Write superblock
    if (write_superblock(map, disk_size, max_files, num_total_blocks, num_inodes, num_data_blocks, num_bitmap_blocks) < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Initialize inode table
    if (initialize_inode_table(num_inodes) < 0) {
        cleanup_disk(map, disk_size, fd);
        return -1;
    }

    // Initialize data bitmap
    if (initialize_bitmap(num_bitmap_blocks) < 0) {
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
    size_t num_total_blocks, size_t num_inode_blocks, size_t num_data_blocks, 
    size_t num_bitmap_blocks) {
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

    superblock_t superblock;
    assert(sizeof(superblock) <= BLOCK_SIZE);   // superblock needs to fit in a block
    superblock.magic = VSFS_MAGIC;
    superblock.disk_size = disk_size;
    superblock.block_size = BLOCK_SIZE;
    superblock.num_total_blocks = num_total_blocks;
    superblock.num_inode_blocks = num_inode_blocks;
    superblock.num_data_blocks = num_data_blocks;
    superblock.num_bitmap_blocks = num_bitmap_blocks;
    superblock.num_max_inodes = max_files;
    superblock.next_free_inode = 1;
    superblock.num_free_blocks = num_data_blocks;
    superblock.root_inode = 0;
    memcpy(disk_map, &superblock, sizeof(superblock));

    fs_ctx.sb = superblock;
    
    return 0;
}

int initialize_inode_table() {
    // TODO: Implement inode table initialization
    // - Calculate starting block for inode table (after superblock)
    // - Initialize all inodes to zero (already done by memset)
    // - All inodes are free initially (mode = 0 means free)
    // - Set up inode 0 for root directory (will be done in create_root_directory)
    // - No inode bitmap needed - using fixed allocation with next_free_inode counter
    // - Return 0 on success, -1 on error

    memset(fs_ctx.inode_table, 0, fs_ctx.sb.num_inode_blocks * BLOCK_SIZE);
    
    return 0;
}

int initialize_bitmap() {
    // TODO: Implement data bitmap initialization
    // - Calculate starting block for data bitmap (after inode table)
    // - Initialize bitmap to all zeros (all blocks free)
    // - Mark block 0 as used (if needed for special purposes)
    // - Each bit represents one data block (0 = free, 1 = used)
    // - Return 0 on success, -1 on error

    // Clear the entire bitmap (set all bits to 0)
    if (bitmapclear(fs_ctx.bitmap, fs_ctx.sb.num_total_blocks) < 0) {
        return -1;
    }
    
    // Mark block 0 (superblock) as used
    if (bitmapset(fs_ctx.bitmap, fs_ctx.sb.num_total_blocks, 0, true) < 0) {
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

    inode_t root_inode;
    root_inode.size = 0;
    root_inode.atime = time(NULL);
    root_inode.mtime = time(NULL);
    root_inode.ctime = time(NULL);
    root_inode.nlinks = 2;
    root_inode.blocks[0] = 0;   // point at superblock to imply unused
    root_inode.indirect = 0;    // point at first inode to imply unused
    memcpy(fs_ctx.inode_table, &root_inode, sizeof(root_inode));
    
    return 0;
}

int calculate_layout(char *disk_map, size_t disk_size, size_t max_files, 
                    size_t *num_total_blocks, size_t *num_inode_blocks, size_t *num_data_blocks, 
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
    *num_total_blocks = disk_size / BLOCK_SIZE;
    *num_bitmap_blocks = ceildiv(*num_total_blocks, BLOCK_SIZE * 8);
    *num_inode_blocks = (max_files * INODE_SIZE) / BLOCK_SIZE;
    *num_data_blocks = *num_total_blocks - num_superblock_blocks - *num_inode_blocks - *num_bitmap_blocks;

    assert(*num_bitmap_blocks >= 1);    // superblock must exist, so at least 1 bitmap block to keep track of superblock
    assert(*num_total_blocks == num_superblock_blocks + *num_bitmap_blocks + *num_inode_blocks + *num_data_blocks);

    fs_ctx.disk_map = disk_map;
    fs_ctx.bitmap = disk_map + BLOCK_SIZE;
    fs_ctx.inode_table = disk_map + BLOCK_SIZE + *num_bitmap_blocks * BLOCK_SIZE;
    fs_ctx.data_section = disk_map + BLOCK_SIZE + *num_bitmap_blocks * BLOCK_SIZE + *num_inode_blocks * BLOCK_SIZE;

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
    
    // Clear the filesystem context
    memset(&fs_ctx, 0, sizeof(fs_ctx_t));
}