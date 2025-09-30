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

/**
 * Write from a buffer to a data block
 * 
 * Updates the inode and bytes_written accordingly.
 * 
 * @param inode Inode of the file to write to
 * @param block_index Index of the block to write to. Assumes this is a newly allocated data block
 * @param buffer Buffer to write from
 * @param buffer_size Size of the buffer
 * @param bytes_written Number of bytes already written to the buffer. Updated by this function
 * 
 */
uint32_t write_to_datablock(inode_t* inode, uint32_t block_index, const char* buffer, uint32_t buffer_size, uint32_t* bytes_written);