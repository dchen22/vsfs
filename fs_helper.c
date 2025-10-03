#include "fs_helper.h"

inode_t* get_inode_by_name(const char *filename, uint32_t* inode_index) {
    // linear search through files
    for (unsigned int i = 0; i < sb->num_max_inodes; i++) {
        if (bitmapget(inode_bitmap, sb->num_max_inodes, i)) {
            inode_t* inode = (inode_t*)(inode_table + i * sizeof(inode_t));
            if (strcmp(inode->name, filename) == 0) {
                if (inode_index != NULL) {
                    *inode_index = i;
                }
                return inode;
            }
        }
    }

    return NULL;
}

inode_t* get_inode_by_index(uint32_t inode_index) {

    if (bitmapget(inode_bitmap, sb->num_max_inodes, inode_index) == 1) {
        inode_t* inode = (inode_t*)(inode_table + inode_index * sizeof(inode_t));
        if (!inode->is_allocated) { // sanity check
            printf("GET_INODE_BY_INDEX FAILURE: Inode allocated in bitmap but is_allocated=false\n");
            return NULL;
        }
        return inode;
    }
    return NULL;
}

uint32_t write_to_datablock(inode_t* inode, uint32_t block_index, const char* buffer, uint32_t buffer_size, uint32_t* bytes_written) {
    if (buffer_size < *bytes_written) {
        printf("WRITE_TO_DATABLOCK FAILURE: bytes_read is greater than buffer_size\n");
        return 0;
    }

    if (buffer_size == *bytes_written) {
        return 0;
    }

    if (buffer_size - *bytes_written >= BLOCK_SIZE) {
        memcpy(disk_start + block_index * BLOCK_SIZE, buffer + *bytes_written, BLOCK_SIZE);
        inode->size += BLOCK_SIZE;
        *bytes_written += BLOCK_SIZE;
        return BLOCK_SIZE;
    } else {
        memcpy(disk_start + block_index * BLOCK_SIZE, buffer + *bytes_written, buffer_size - *bytes_written);
        inode->size += buffer_size - *bytes_written;
        *bytes_written += buffer_size - *bytes_written;
        return buffer_size - *bytes_written;
    }

    printf("WRITE_TO_DATABLOCK FAILURE: Failed to write to data block\n");
    return 0;
}

void print_files_in_dir(inode_t* directory) {
    if (!directory->is_directory) {
        printf("Not a directory\n");
        return;
    }
    inode_t** files = get_files_in_dir(directory);
    if (files == NULL) {
        printf("PRINT_FILES_IN_DIR FAILURE: get_files_in_dir returned NULL\n");
        return;
    }
    
    // Print all files
    for (int i = 0; files[i] != NULL; i++) {
        printf("%s\n", files[i]->name);
    }
    
    // Free the array
    free_get_files_in_dir(files);
}

inode_t** get_files_in_dir(inode_t* directory) {
    if (!directory->is_directory) {
        printf("GET_FILES_IN_DIR FAILURE: Not a directory\n");
        return NULL;
    }
    
    // First pass: count the number of files
    uint32_t file_count = 0;
    
    // Count files in direct blocks
    for (uint32_t i = 0; i < 12; i++) {
        if (directory->direct_blocknums[i] != 0) {
            uint32_t* direct_block = (uint32_t*)(disk_start + directory->direct_blocknums[i] * BLOCK_SIZE);
            for (uint32_t d = 0; d < BLOCK_SIZE / sizeof(uint32_t); d++) {
                if (direct_block[d] != 0) {
                    file_count++;
                }
            }
        }
    }
    
    // Count files in indirect blocks
    if (directory->indirect_blocknum != 0) {
        uint32_t* indirect_block = (uint32_t*)(disk_start + directory->indirect_blocknum * BLOCK_SIZE);
        for (uint32_t i = 0; i < BLOCK_SIZE / sizeof(uint32_t); i++) {
            if (indirect_block[i] != 0) {
                uint32_t* directory_block = (uint32_t*)(disk_start + indirect_block[i] * BLOCK_SIZE);
                for (uint32_t d = 0; d < BLOCK_SIZE / sizeof(uint32_t); d++) {
                    if (directory_block[d] != 0) {
                        file_count++;
                    }
                }
            }
        }
    }
    
    // Allocate array for inode pointers (+1 for NULL terminator)
    inode_t** files = (inode_t**)malloc((file_count + 1) * sizeof(inode_t*));
    if (files == NULL) {
        printf("GET_FILES_IN_DIR FAILURE: Memory allocation failed\n");
        return NULL;
    }
    
    // Initialize the array
    for (uint32_t i = 0; i <= file_count; i++) {
        files[i] = NULL;
    }
    
    // Second pass: populate the array
    uint32_t file_index = 0;
    
    // Add files from direct blocks
    for (uint32_t i = 0; i < 12; i++) {
        if (directory->direct_blocknums[i] != 0) {
            uint32_t* direct_block = (uint32_t*)(disk_start + directory->direct_blocknums[i] * BLOCK_SIZE);
            for (uint32_t d = 0; d < BLOCK_SIZE / sizeof(uint32_t); d++) {
                if (direct_block[d] != 0) {
                    inode_t* subfile = get_inode_by_index(direct_block[d]);
                    if (subfile == NULL) {
                        printf("GET_FILES_IN_DIR FAILURE: Found subfile is null\n");
                        free(files);
                        return NULL;
                    }
                    files[file_index++] = subfile;
                }
            }
            
        }
    }
    
    // Add files from indirect blocks
    if (directory->indirect_blocknum != 0) {
        uint32_t* indirect_block = (uint32_t*)(disk_start + directory->indirect_blocknum * BLOCK_SIZE);
        for (uint32_t i = 0; i < BLOCK_SIZE / sizeof(uint32_t); i++) {
            if (indirect_block[i] != 0) {
                uint32_t* directory_block = (uint32_t*)(disk_start + indirect_block[i] * BLOCK_SIZE);
                for (uint32_t d = 0; d < BLOCK_SIZE / sizeof(uint32_t); d++) {
                    if (directory_block[d] != 0) {
                        inode_t* subfile = get_inode_by_index(directory_block[d]);
                        if (subfile == NULL) {
                            printf("GET_FILES_IN_DIR FAILURE: Found subfile is null\n");
                            free(files);
                            return NULL;
                        }
                        files[file_index++] = subfile;
                    }
                }
                
            }
        }
    }
    
    return files;
}

void free_get_files_in_dir(inode_t** files) {
    if (files != NULL) {
        free(files);
    }
}
