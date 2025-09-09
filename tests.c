#include "fs.h"
#include "mkfs.h"

int main() {
    if (format_disk("disk", BLOCK_SIZE * 100, 1000) < 0) {
        printf("Failed to format disk.\n");
        return -1; // Return -1 on error
    }


    printf("Disk formatted successfully.\n");
    return 0;
}