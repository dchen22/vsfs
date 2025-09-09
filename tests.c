#include "fs.h"
#include "mkfs.h"
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>

// Define VSFS_MAGIC for testing
#define VSFS_MAGIC 0x56534653 // "VSFS" in hex

// Test helper functions
int test_superblock_validity(const char *disk_name);
int test_filesystem_layout(const char *disk_name, size_t expected_disk_size, size_t expected_max_files);
int test_inode_table_initialization(const char *disk_name);
int test_bitmap_initialization(const char *disk_name);
int test_root_directory_creation(const char *disk_name);
int test_disk_file_creation(const char *disk_name, size_t expected_size);

// Test cases
int test_basic_formatting();
int test_different_disk_sizes();
int test_different_max_files();
int test_edge_cases();

int main() {
    printf("=== VSFS Filesystem Setup Tests ===\n\n");
    
    // Test 1: Basic formatting
    printf("Test 1: Basic filesystem formatting\n");
    if (test_basic_formatting() == 0) {
        printf("✓ Basic formatting test passed\n");
    } else {
        printf("✗ Basic formatting test failed\n");
        printf("❌ Test suite terminated due to failure\n");
        return -1;
    }
    printf("\n");
    
    // Test 2: Different disk sizes
    printf("Test 2: Different disk sizes\n");
    if (test_different_disk_sizes() == 0) {
        printf("✓ Different disk sizes test passed\n");
    } else {
        printf("✗ Different disk sizes test failed\n");
        printf("❌ Test suite terminated due to failure\n");
        return -1;
    }
    printf("\n");
    
    // Test 3: Different max files
    printf("Test 3: Different max files\n");
    if (test_different_max_files() == 0) {
        printf("✓ Different max files test passed\n");
    } else {
        printf("✗ Different max files test failed\n");
        printf("❌ Test suite terminated due to failure\n");
        return -1;
    }
    printf("\n");
    
    // Test 4: Edge cases
    printf("Test 4: Edge cases\n");
    if (test_edge_cases() == 0) {
        printf("✓ Edge cases test passed\n");
    } else {
        printf("✗ Edge cases test failed\n");
        printf("❌ Test suite terminated due to failure\n");
        return -1;
    }
    printf("\n");
    
    // All tests passed
    printf("=== Test Summary ===\n");
    printf("🎉 All tests passed!\n");
    return 0;
}

int test_basic_formatting() {
    const char *disk_name = "test_disk_basic";
    size_t disk_size = BLOCK_SIZE * 100;  // 100 blocks
    size_t max_files = 100;
    
    // Clean up any existing test file
    unlink(disk_name);
    
    // Format the disk (this creates the file)
    if (format_disk(disk_name, disk_size, max_files) < 0) {
        printf("    ✗ Failed to format disk\n");
        return -1;
    }
    
    // Test disk file creation
    if (test_disk_file_creation(disk_name, disk_size) != 0) {
        return -1;
    }
    
    // Test superblock validity
    if (test_superblock_validity(disk_name) != 0) {
        return -1;
    }
    
    // Test filesystem layout
    if (test_filesystem_layout(disk_name, disk_size, max_files) != 0) {
        return -1;
    }
    
    // Test inode table initialization
    if (test_inode_table_initialization(disk_name) != 0) {
        return -1;
    }
    
    // Test bitmap initialization
    if (test_bitmap_initialization(disk_name) != 0) {
        return -1;
    }
    
    // Test root directory creation
    if (test_root_directory_creation(disk_name) != 0) {
        return -1;
    }
    
    // Clean up
    unlink(disk_name);
    return 0;
}

int test_different_disk_sizes() {
    const char *disk_name = "test_disk_sizes";
    // All sizes must be >= 2 * BLOCK_SIZE (minimum requirement)
    size_t test_sizes[] = {BLOCK_SIZE * 2, BLOCK_SIZE * 10, BLOCK_SIZE * 50, BLOCK_SIZE * 200, BLOCK_SIZE * 1000};
    size_t max_files = 50;
    
    for (int i = 0; i < 5; i++) {
        unlink(disk_name);
        
        printf("  Testing disk size: %zu bytes (%zu blocks)\n", 
               test_sizes[i], test_sizes[i] / BLOCK_SIZE);
        
        if (format_disk(disk_name, test_sizes[i], max_files) < 0) {
            printf("    ✗ Failed to format disk with size %zu\n", test_sizes[i]);
            return -1;
        }
        
        if (test_filesystem_layout(disk_name, test_sizes[i], max_files) != 0) {
            printf("    ✗ Filesystem layout test failed for size %zu\n", test_sizes[i]);
            return -1;
        }
        
        printf("    ✓ Disk size %zu formatted successfully\n", test_sizes[i]);
    }
    
    unlink(disk_name);
    return 0;
}

int test_different_max_files() {
    const char *disk_name = "test_disk_files";
    size_t disk_size = BLOCK_SIZE * 100;
    size_t test_max_files[] = {10, 50, 100, 500, 1000};
    
    for (int i = 0; i < 5; i++) {
        unlink(disk_name);
        
        printf("  Testing max files: %zu\n", test_max_files[i]);
        
        if (format_disk(disk_name, disk_size, test_max_files[i]) < 0) {
            printf("    ✗ Failed to format disk with max_files %zu\n", test_max_files[i]);
            return -1;
        }
        
        if (test_filesystem_layout(disk_name, disk_size, test_max_files[i]) != 0) {
            printf("    ✗ Filesystem layout test failed for max_files %zu\n", test_max_files[i]);
            return -1;
        }
        
        printf("    ✓ Max files %zu formatted successfully\n", test_max_files[i]);
    }
    
    unlink(disk_name);
    return 0;
}

int test_edge_cases() {
    const char *disk_name = "test_disk_edge";
    
    // Test 1: Minimum disk size (2 * BLOCK_SIZE)
    printf("  Testing minimum disk size (2 * BLOCK_SIZE)\n");
    unlink(disk_name);
    if (format_disk(disk_name, BLOCK_SIZE * 2, 1) < 0) {
        printf("    ✗ Failed to format minimum disk size\n");
        return -1;
    }
    printf("    ✓ Minimum disk size formatted successfully\n");
    
    // Test 2: Disk size smaller than minimum (should fail)
    printf("  Testing disk size smaller than minimum (should fail)\n");
    unlink(disk_name);
    if (format_disk(disk_name, BLOCK_SIZE, 1) == 0) {
        printf("    ✗ Should have failed with disk size smaller than 2 * BLOCK_SIZE\n");
        return -1;
    }
    printf("    ✓ Correctly failed with disk size smaller than minimum\n");
    
    // Test 3: Maximum files (limited by MAX_INODES)
    printf("  Testing maximum files\n");
    unlink(disk_name);
    if (format_disk(disk_name, BLOCK_SIZE * 1000, MAX_INODES) < 0) {
        printf("    ✗ Failed to format with maximum files\n");
        return -1;
    }
    printf("    ✓ Maximum files formatted successfully\n");
    
    // Test 4: Zero max files (should fail)
    printf("  Testing zero max files (should fail)\n");
    unlink(disk_name);
    if (format_disk(disk_name, BLOCK_SIZE * 100, 0) == 0) {
        printf("    ✗ Should have failed with zero max files\n");
        return -1;
    }
    printf("    ✓ Correctly failed with zero max files\n");
    
    unlink(disk_name);
    return 0;
}

int test_disk_file_creation(const char *disk_name, size_t expected_size) {
    struct stat st;
    
    if (stat(disk_name, &st) != 0) {
        printf("    ✗ Disk file was not created\n");
        return -1;
    }
    
    if ((size_t)st.st_size != expected_size) {
        printf("    ✗ Disk file size mismatch: expected %zu, got %ld\n", 
               expected_size, st.st_size);
        return -1;
    }
    
    printf("    ✓ Disk file created with correct size\n");
    return 0;
}

int test_superblock_validity(const char *disk_name) {
    int fd = open(disk_name, O_RDONLY);
    if (fd < 0) {
        printf("    ✗ Failed to open disk file for reading\n");
        return -1;
    }
    
    superblock_t sb;
    if (read(fd, &sb, sizeof(sb)) != sizeof(sb)) {
        printf("    ✗ Failed to read superblock\n");
        close(fd);
        return -1;
    }
    
    // Test magic number
    if (sb.magic != VSFS_MAGIC) {
        printf("    ✗ Invalid magic number: 0x%08x (expected 0x%08x)\n", 
               sb.magic, VSFS_MAGIC);
        close(fd);
        return -1;
    }
    
    // Test block size
    if (sb.block_size != BLOCK_SIZE) {
        printf("    ✗ Invalid block size: %u (expected %d)\n", 
               sb.block_size, BLOCK_SIZE);
        close(fd);
        return -1;
    }
    
    // Test root inode
    if (sb.root_inode != 0) {
        printf("    ✗ Invalid root inode: %u (expected 0)\n", sb.root_inode);
        close(fd);
        return -1;
    }
    
    // Test next free inode
    if (sb.next_free_inode != 1) {
        printf("    ✗ Invalid next free inode: %u (expected 1)\n", sb.next_free_inode);
        close(fd);
        return -1;
    }
    
    printf("    ✓ Superblock is valid\n");
    close(fd);
    return 0;
}

int test_filesystem_layout(const char *disk_name, size_t expected_disk_size, size_t expected_max_files) {
    int fd = open(disk_name, O_RDONLY);
    if (fd < 0) {
        printf("    ✗ Failed to open disk file\n");
        return -1;
    }
    
    superblock_t sb;
    if (read(fd, &sb, sizeof(sb)) != sizeof(sb)) {
        printf("    ✗ Failed to read superblock\n");
        close(fd);
        return -1;
    }
    
    // Test disk size
    if (sb.disk_size != expected_disk_size) {
        printf("    ✗ Disk size mismatch: %u (expected %zu)\n", 
               sb.disk_size, expected_disk_size);
        close(fd);
        return -1;
    }
    
    // Test max files
    if (sb.num_max_inodes != expected_max_files) {
        printf("    ✗ Max files mismatch: %u (expected %zu)\n", 
               sb.num_max_inodes, expected_max_files);
        close(fd);
        return -1;
    }
    
    // Test total blocks calculation
    size_t expected_total_blocks = expected_disk_size / BLOCK_SIZE;
    if (sb.num_total_blocks != expected_total_blocks) {
        printf("    ✗ Total blocks mismatch: %u (expected %zu)\n", 
               sb.num_total_blocks, expected_total_blocks);
        close(fd);
        return -1;
    }
    
    // Test layout consistency
    size_t calculated_total = 1 + sb.num_bitmap_blocks + sb.num_inode_blocks + sb.num_data_blocks;
    if (calculated_total != sb.num_total_blocks) {
        printf("    ✗ Layout inconsistency: 1 + %u + %u + %u = %zu != %u\n",
               sb.num_bitmap_blocks, sb.num_inode_blocks, sb.num_data_blocks,
               calculated_total, sb.num_total_blocks);
        close(fd);
        return -1;
    }
    
    printf("    ✓ Filesystem layout is consistent\n");
    close(fd);
    return 0;
}

int test_inode_table_initialization(const char *disk_name) {
    int fd = open(disk_name, O_RDONLY);
    if (fd < 0) {
        printf("    ✗ Failed to open disk file\n");
        return -1;
    }
    
    superblock_t sb;
    if (read(fd, &sb, sizeof(sb)) != sizeof(sb)) {
        printf("    ✗ Failed to read superblock\n");
        close(fd);
        return -1;
    }
    
    // Seek to inode table
    if (lseek(fd, BLOCK_SIZE + sb.num_bitmap_blocks * BLOCK_SIZE, SEEK_SET) == -1) {
        printf("    ✗ Failed to seek to inode table\n");
        close(fd);
        return -1;
    }
    
    // Read first inode (should be root directory)
    inode_t root_inode;
    if (read(fd, &root_inode, sizeof(root_inode)) != sizeof(root_inode)) {
        printf("    ✗ Failed to read root inode\n");
        close(fd);
        return -1;
    }
    
    // Test root inode properties
    if (root_inode.size != 0) {
        printf("    ✗ Root inode size should be 0, got %u\n", root_inode.size);
        close(fd);
        return -1;
    }
    
    if (root_inode.nlinks != 2) {
        printf("    ✗ Root inode nlinks should be 2, got %u\n", root_inode.nlinks);
        close(fd);
        return -1;
    }
    
    printf("    ✓ Inode table initialized correctly\n");
    close(fd);
    return 0;
}

int test_bitmap_initialization(const char *disk_name) {
    int fd = open(disk_name, O_RDONLY);
    if (fd < 0) {
        printf("    ✗ Failed to open disk file\n");
        return -1;
    }
    
    superblock_t sb;
    if (read(fd, &sb, sizeof(sb)) != sizeof(sb)) {
        printf("    ✗ Failed to read superblock\n");
        close(fd);
        return -1;
    }
    
    // Seek to bitmap
    if (lseek(fd, BLOCK_SIZE, SEEK_SET) == -1) {
        printf("    ✗ Failed to seek to bitmap\n");
        close(fd);
        return -1;
    }
    
    // Read bitmap data
    size_t bitmap_size = sb.num_bitmap_blocks * BLOCK_SIZE;
    char *bitmap_data = malloc(bitmap_size);
    if (!bitmap_data) {
        printf("    ✗ Failed to allocate memory for bitmap\n");
        close(fd);
        return -1;
    }
    
    if (read(fd, bitmap_data, bitmap_size) != (ssize_t)bitmap_size) {
        printf("    ✗ Failed to read bitmap data\n");
        free(bitmap_data);
        close(fd);
        return -1;
    }
    
    // Test that block 0 (superblock) is marked as used
    int block0_status = bitmapget(bitmap_data, sb.num_total_blocks, 0);
    if (block0_status != 1) {
        printf("    ✗ Block 0 (superblock) should be marked as used, got %d\n", block0_status);
        free(bitmap_data);
        close(fd);
        return -1;
    }
    
    // Test that other blocks are free (0)
    int free_blocks = 0;
    for (size_t i = 1; i < sb.num_total_blocks && i < 100; i++) { // Test first 100 blocks
        int status = bitmapget(bitmap_data, sb.num_total_blocks, i);
        if (status == 0) {
            free_blocks++;
        } else if (status < 0) {
            printf("    ✗ Error reading bitmap at index %zu\n", i);
            free(bitmap_data);
            close(fd);
            return -1;
        }
    }
    
    // Most blocks should be free initially
    if (free_blocks < 90) { // At least 90% should be free
        printf("    ✗ Too few free blocks found: %d\n", free_blocks);
        free(bitmap_data);
        close(fd);
        return -1;
    }
    
    printf("    ✓ Bitmap initialized correctly (block 0 used, others free)\n");
    free(bitmap_data);
    close(fd);
    return 0;
}

int test_root_directory_creation(const char *disk_name) {
    int fd = open(disk_name, O_RDONLY);
    if (fd < 0) {
        printf("    ✗ Failed to open disk file\n");
        return -1;
    }
    
    superblock_t sb;
    if (read(fd, &sb, sizeof(sb)) != sizeof(sb)) {
        printf("    ✗ Failed to read superblock\n");
        close(fd);
        return -1;
    }
    
    // Test that root inode is properly set in superblock
    if (sb.root_inode != 0) {
        printf("    ✗ Root inode should be 0, got %u\n", sb.root_inode);
        close(fd);
        return -1;
    }
    
    // Test that next free inode is set correctly
    if (sb.next_free_inode != 1) {
        printf("    ✗ Next free inode should be 1, got %u\n", sb.next_free_inode);
        close(fd);
        return -1;
    }
    
    printf("    ✓ Root directory created correctly\n");
    close(fd);
    return 0;
}
