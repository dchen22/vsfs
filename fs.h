#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// Function declarations for VSFS filesystem operations
int load_fs(const char *disk_name);
void unload_fs(void);
int create_file(const char *filename);
int delete_file(const char *filename);

/**
 * Print all files in the filesystem
 * 
 * @return Number of files printed 
 */
int print_all_files(void);
void print_fs_status(void);