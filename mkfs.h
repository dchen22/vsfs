#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define BLOCK_SIZE 4096 // Size of a block in bytes

typedef struct {
    size_t disk_size;
    size_t num_inodes;
} fs_t;