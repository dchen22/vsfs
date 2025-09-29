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