#include "fs.h"
#include "mkfs.h"

int main() {
    // Simple example: format a disk and load it
    printf("VSFS Example Program\n");
    
    // Format a disk
    if (format_disk("disk", BLOCK_SIZE * 100, 1000) < 0) {
        printf("Failed to format disk\n");
        return -1;
    }
    printf("Disk formatted successfully\n");
    
    // Load the filesystem
    if (load_fs("disk") < 0) {
        printf("Failed to load filesystem\n");
        return -1;
    }
    
    // Unload the filesystem
    unload_fs();
    printf("Filesystem unloaded successfully\n");
    
    return 0;
}