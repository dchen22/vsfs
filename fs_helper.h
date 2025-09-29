#include "mkfs.h"
#include "helpers.h"

/**
 * Get an inode by name
 * 
 * @param filename Name of the file to get
 * @param inode_index Pointer to store index of this inode. Leave NULL if not needed
 * 
 * @return Pointer to inode if found, NULL if not found
 */
inode_t* get_inode_by_name(const char *filename, uint32_t* inode_index);