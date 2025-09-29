#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>


// Function declarations for VSFS filesystem operations
int load_fs(const char *disk_name);
void unload_fs(void);
int create_file(const char *filename);
int delete_file(const char *filename);

/**
 * Read a file's contents into a buffer
 * 
 * @param filename The name of the file to read
 * @param buffer The buffer to read the file into
 * @param buffer_size The size of the buffer
 * @return Number of bytes read (0 may indicate empty file or error)
 */
uint32_t read_file(const char *filename, char *buffer, unsigned long buffer_size);

/**
 * Print all files in the filesystem
 * 
 * @return Number of files printed 
 */
unsigned int print_all_files(void);
void print_fs_status(void);