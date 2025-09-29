#include "fs.h"
#include "fs_helper.h"
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
    // increment root directory's nlinks
    inode_t* root_inode = (inode_t*)(inode_table);
    root_inode->nlinks++;
    for (unsigned int i = 0; i < 12; i++) {
        new_inode->direct_blocks[i] = 0;    // points at 0th disk block (superblock), not 0th data block
    }
    new_inode->indirect_block = 0;
    sb->num_used_inodes++;

    return 0;
}

int delete_file(const char *filename) {
    // get root inode
    inode_t* root_inode = (inode_t*)(inode_table);
    if (root_inode == NULL ||root_inode->is_allocated == false) {
        printf("FAILURE: Root inode not found\n");
        return -1;
    }
    
    uint32_t inode_index;
    inode_t* inode = get_inode_by_name(filename, &inode_index);
    if (inode == NULL) {
        printf("FAILURE: File not found\n");
        return -1;
    }
    // TODO: if deleting directory, need to delete subfiles too

    // update superblock
    sb->num_used_inodes--;
    // decrement root directory's nlinks
    root_inode->nlinks--;
    // set inode is_allocated to false
    inode->is_allocated = false; 
    // free inode
    bitmapset(inode_bitmap, sb->num_max_inodes, inode_index, 0);
    return 0;
  
}

uint32_t read_file(const char *filename, char *buffer, unsigned long buffer_size) {
    // linear search through files
    inode_t* inode = get_inode_by_name(filename, NULL);
    if (inode == NULL) {
        printf("FAILURE: File not found\n");
        return -1;
    }
    
    if (inode->is_directory) {
        printf("FAILURE: File is a directory\n");
        return -1;
    }

    unsigned long bytes_read = 0;

    // iterate through direct block pointers
    for (unsigned int i = 0; i < 12; i++) {
        // no more direct blocks
        if (inode->direct_blocks[i] == 0) {
            break;
        }

        // read block 
        char* block = (char*)(data_section + inode->direct_blocks[i] * BLOCK_SIZE);

        // copy block to buffer
        if (bytes_read + BLOCK_SIZE <= buffer_size) {   // buffer can still fit at least one block
            // fully copy block
            memcpy(buffer + bytes_read, block, BLOCK_SIZE);
            bytes_read += BLOCK_SIZE;
        } else {    // buffer cannot fit another block
            // partially copy block
            memcpy(buffer + bytes_read, block, buffer_size - bytes_read);
            bytes_read += buffer_size - bytes_read;
            // debug
            if (bytes_read != buffer_size) {
                printf("WARNING: read direct block bytes does not match with buffer size \n");
            }
            return bytes_read;
        }
        
    }

    // read indirect block
    if (inode->indirect_block != 0) {
        uint32_t* indirect_block = (uint32_t*)(data_section + inode->indirect_block * BLOCK_SIZE);
        for (uint32_t i = 0; i < BLOCK_SIZE / sizeof(uint32_t); i++ ) {
            if (indirect_block[i] == 0) {
                break;
            }
            char* block = (char*)(data_section + indirect_block[i] * BLOCK_SIZE);
            if (bytes_read + BLOCK_SIZE <= buffer_size) {   // buffer can still fit at least one block
                // copy full block
                memcpy(buffer + bytes_read, block, BLOCK_SIZE);
                bytes_read += BLOCK_SIZE;
            } else {    // buffer cannot fit another block
                // partially copy block
                memcpy(buffer + bytes_read, block, buffer_size - bytes_read);
                bytes_read += buffer_size - bytes_read;
                // debug
                if (bytes_read != buffer_size) {
                    printf("WARNING: read indirect block bytes does not match with buffer size \n");
                }
                return bytes_read;
            }
        }
    }

    
    return bytes_read;

}


unsigned int print_all_files(void) {
    unsigned int count = 0;
    for (unsigned int i = 0; i < sb->num_max_inodes; i++) {
        if (bitmapget(inode_bitmap, sb->num_max_inodes, i) == 1) {
            inode_t *inode = (inode_t*)(inode_table + i * sizeof(inode_t));
            printf("%s\n", inode->name);
            count++;
        }
    }
    return count;
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