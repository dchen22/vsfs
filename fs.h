#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// Function declarations for VSFS filesystem operations
int load_fs(const char *disk_name);
void unload_fs(void);
int create_file(const char *filename);
int print_all_files(void);
void print_fs_status(void);