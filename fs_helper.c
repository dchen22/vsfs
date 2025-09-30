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

uint32_t write_to_datablock(inode_t* inode, uint32_t block_index, const char* buffer, uint32_t buffer_size, uint32_t* bytes_written) {
    if (buffer_size < *bytes_written) {
        printf("WRITE_TO_DATABLOCK FAILURE: bytes_read is greater than buffer_size\n");
        return 0;
    }

    if (buffer_size == *bytes_written) {
        return 0;
    }

    if (buffer_size - *bytes_written >= BLOCK_SIZE) {
        memcpy(data_section + block_index * BLOCK_SIZE, buffer + *bytes_written, BLOCK_SIZE);
        inode->size += BLOCK_SIZE;
        *bytes_written += BLOCK_SIZE;
        return BLOCK_SIZE;
    } else {
        memcpy(data_section + block_index * BLOCK_SIZE, buffer + *bytes_written, buffer_size - *bytes_written);
        inode->size += buffer_size - *bytes_written;
        *bytes_written += buffer_size - *bytes_written;
        return buffer_size - *bytes_written;
    }

    printf("WRITE_TO_DATABLOCK FAILURE: Failed to write to data block\n");
    return 0;
}
