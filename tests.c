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

// Helper function to calculate minimum blocks needed for a filesystem
size_t calculate_minimum_blocks(size_t max_files);

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
    
    printf("  Beginning tests with block size = %d, inode size = %d\n\n", BLOCK_SIZE, INODE_SIZE);
    
    // Test different disk sizes with appropriate max_files for each
    struct {
        size_t disk_size;
        size_t max_files;
        bool should_succeed;
    } test_cases[] = {
        {BLOCK_SIZE * 4, 1, true},      // Minimum size: superblock + inode_bitmap + data_bitmap + inode_table (1 inode)
        {BLOCK_SIZE * 2, 1, false},     // Too small for even 1 file
        {BLOCK_SIZE * 2, 10, false},    // Too many files for 2 blocks
        {BLOCK_SIZE * 10, 10, true},    // Reasonable size for 10 files
        {BLOCK_SIZE * 50, 50, true},    // Good size for 50 files
        {BLOCK_SIZE * 200, 100, true},  // Large disk with moderate files
        {BLOCK_SIZE * 1000, 500, true}  // Very large disk
    };
    
    for (int i = 0; i < 7; i++) {
        unlink(disk_name);
        
        size_t available_blocks = test_cases[i].disk_size / BLOCK_SIZE;
        size_t min_blocks = calculate_minimum_blocks(test_cases[i].max_files);
        
        // Calculate individual components for detailed output
        size_t num_superblock_blocks = 1;
        size_t num_inode_bitmap_blocks = ceildiv(test_cases[i].max_files, BLOCK_SIZE * 8);
        size_t num_inode_table_blocks = ceildiv(test_cases[i].max_files * INODE_SIZE, BLOCK_SIZE);
        size_t num_data_bitmap_blocks = ceildiv(min_blocks, BLOCK_SIZE * 8);
        
        printf("  Test %d: Let disk size = %zu, thus number of blocks = %zu. Let max files = %zu\n", 
               i, test_cases[i].disk_size, available_blocks, test_cases[i].max_files);
        printf("    Requires: %zu superblock, %zu inode bitmap blocks (ceil(%zu / %zu)), %zu data bitmap blocks (ceil(%zu / %zu)), %zu inode table blocks (ceil(%zu / %zu))\n",
               num_superblock_blocks, 
               num_inode_bitmap_blocks, test_cases[i].max_files, (size_t)(BLOCK_SIZE * 8),
               num_data_bitmap_blocks, min_blocks, (size_t)(BLOCK_SIZE * 8),
               num_inode_table_blocks, test_cases[i].max_files * INODE_SIZE, (size_t)BLOCK_SIZE);
        printf("    Total required: %zu blocks, available: %zu blocks\n", min_blocks, available_blocks);
        
        int result = format_disk(disk_name, test_cases[i].disk_size, test_cases[i].max_files);
        
        if (test_cases[i].should_succeed) {
            if (result < 0) {
                printf("    ✗ Test FAILS - Failed to format disk (should succeed)\n");
                return -1;
            }
            
            if (test_filesystem_layout(disk_name, test_cases[i].disk_size, test_cases[i].max_files) != 0) {
                printf("    ✗ Test FAILS - Filesystem layout test failed\n");
                return -1;
            }
            
            printf("    ✓ Test PASSES - Disk formatted successfully\n");
        } else {
            if (result >= 0) {
                printf("    ✗ Test FAILS - Should have failed to format disk (too many files for disk size)\n");
                return -1;
            }
            
            printf("    ✓ Test PASSES - Correctly failed to format disk (insufficient space)\n");
        }
        printf("\n");
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
    
    // Test 1: Minimum disk size (4 * BLOCK_SIZE for 1 file)
    printf("  Testing minimum disk size (4 * BLOCK_SIZE for 1 file)\n");
    unlink(disk_name);
    if (format_disk(disk_name, BLOCK_SIZE * 4, 1) < 0) {
        printf("    ✗ Failed to format minimum disk size\n");
        return -1;
    }
    printf("    ✓ Minimum disk size formatted successfully\n");
    
    // Test 2: Disk size smaller than minimum (should fail)
    printf("  Testing disk size smaller than minimum (should fail)\n");
    unlink(disk_name);
    if (format_disk(disk_name, BLOCK_SIZE * 2, 1) == 0) {
        printf("    ✗ Should have failed with disk size smaller than 4 * BLOCK_SIZE\n");
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
    
    // Test used inodes (should be 1 after root directory creation)
    if (sb.num_used_inodes != 1) {
        printf("    ✗ Invalid used inodes: %u (expected 1)\n", sb.num_used_inodes);
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
    size_t calculated_total = 1 + sb.num_inode_bitmap_blocks + sb.num_data_bitmap_blocks + sb.num_inode_table_blocks + sb.num_data_blocks;
    if (calculated_total != sb.num_total_blocks) {
        printf("    ✗ Layout inconsistency: 1 + %u + %u + %u + %u = %zu != %u\n",
               sb.num_inode_bitmap_blocks, sb.num_data_bitmap_blocks, sb.num_inode_table_blocks, sb.num_data_blocks,
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
    if (lseek(fd, BLOCK_SIZE + sb.num_inode_bitmap_blocks * BLOCK_SIZE + sb.num_data_bitmap_blocks * BLOCK_SIZE, SEEK_SET) == -1) {
        printf("    ✗ Failed to seek to inode table\n");
        close(fd);
        return -1;
    }
    
    // Test that inode table is properly initialized (all inodes should be zeroed except root)
    // Read a few inodes to verify they are zeroed (except inode 0 which is the root)
    for (int i = 1; i < 5 && i < (int)sb.num_max_inodes; i++) {
        if (lseek(fd, i * sizeof(inode_t), SEEK_CUR) == -1) {
            printf("    ✗ Failed to seek to inode %d\n", i);
            close(fd);
            return -1;
        }
        
        inode_t test_inode;
        if (read(fd, &test_inode, sizeof(test_inode)) != sizeof(test_inode)) {
            printf("    ✗ Failed to read inode %d\n", i);
            close(fd);
            return -1;
        }
        
        // All inodes except root should be zeroed
        if (test_inode.size != 0 || test_inode.nlinks != 0 || 
            test_inode.atime != 0 || test_inode.mtime != 0 || test_inode.ctime != 0) {
            printf("    ✗ Inode %d should be zeroed, but has non-zero values\n", i);
            close(fd);
            return -1;
        }
    }
    
    printf("    ✓ Inode table initialized correctly (free inodes are zeroed)\n");
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
    
    // Seek to data bitmap (after inode bitmap)
    if (lseek(fd, BLOCK_SIZE + sb.num_inode_bitmap_blocks * BLOCK_SIZE, SEEK_SET) == -1) {
        printf("    ✗ Failed to seek to data bitmap\n");
        close(fd);
        return -1;
    }
    
    // Read data bitmap data
    size_t bitmap_size = sb.num_data_bitmap_blocks * BLOCK_SIZE;
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
    
    // Test that metadata blocks are marked as used
    size_t num_metadata_blocks = 1 + sb.num_inode_bitmap_blocks + sb.num_data_bitmap_blocks;
    for (size_t i = 0; i < num_metadata_blocks; i++) {
        int block_status = bitmapget(bitmap_data, sb.num_total_blocks, i);
        if (block_status != 1) {
            printf("    ✗ Block %zu (metadata) should be marked as used, got %d\n", i, block_status);
            free(bitmap_data);
            close(fd);
            return -1;
        }
    }
    
    // Test that data blocks are free (0)
    int free_blocks = 0;
    size_t start_data_blocks = num_metadata_blocks + sb.num_inode_table_blocks;
    size_t end_test = (start_data_blocks + 100 < sb.num_total_blocks) ? start_data_blocks + 100 : sb.num_total_blocks;
    
    for (size_t i = start_data_blocks; i < end_test; i++) {
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
    
    // Most data blocks should be free initially
    size_t num_data_blocks_tested = end_test - start_data_blocks;
    if (num_data_blocks_tested > 0 && free_blocks < (int)(num_data_blocks_tested * 0.9)) {
        printf("    ✗ Too few free data blocks found: %d out of %zu\n", free_blocks, num_data_blocks_tested);
        free(bitmap_data);
        close(fd);
        return -1;
    }
    
    printf("    ✓ Bitmap initialized correctly (metadata blocks used, data blocks free)\n");
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
    
    // Seek to inode table
    if (lseek(fd, BLOCK_SIZE + sb.num_inode_bitmap_blocks * BLOCK_SIZE + sb.num_data_bitmap_blocks * BLOCK_SIZE, SEEK_SET) == -1) {
        printf("    ✗ Failed to seek to inode table\n");
        close(fd);
        return -1;
    }
    
    // Read root inode (inode 0)
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
    
    // Test that root inode has valid timestamps
    if (root_inode.atime == 0 || root_inode.mtime == 0 || root_inode.ctime == 0) {
        printf("    ✗ Root inode timestamps should be set\n");
        close(fd);
        return -1;
    }
    
    printf("    ✓ Root directory inode structure is correct\n");
    close(fd);
    return 0;
}

size_t calculate_minimum_blocks(size_t max_files) {
    // Calculate minimum blocks needed for a filesystem with max_files
    // This mirrors the logic in calculate_layout() in mkfs.c
    
    size_t num_superblock_blocks = 1;
    size_t num_inode_bitmap_blocks = ceildiv(max_files, BLOCK_SIZE * 8);
    size_t num_inode_table_blocks = ceildiv(max_files * INODE_SIZE, BLOCK_SIZE);
    
    // For data bitmap, we need to know total blocks, so we calculate iteratively
    // Start with minimum estimate
    size_t total_blocks = num_superblock_blocks + num_inode_bitmap_blocks + num_inode_table_blocks + 1; // +1 for data bitmap
    
    // Recalculate data bitmap with the estimated total
    size_t num_data_bitmap_blocks = ceildiv(total_blocks, BLOCK_SIZE * 8);
    
    // Recalculate total with correct data bitmap size
    total_blocks = num_superblock_blocks + num_inode_bitmap_blocks + num_inode_table_blocks + num_data_bitmap_blocks;
    
    // If data bitmap size changed, recalculate once more
    size_t new_data_bitmap_blocks = ceildiv(total_blocks, BLOCK_SIZE * 8);
    if (new_data_bitmap_blocks != num_data_bitmap_blocks) {
        total_blocks = num_superblock_blocks + num_inode_bitmap_blocks + num_inode_table_blocks + new_data_bitmap_blocks;
    }
    
    return total_blocks;
}
