#include "fs.h"
#include "mkfs.h"

int main() {
    format_disk("disk", BLOCK_SIZE * 100, 1000);

    return 0;
}