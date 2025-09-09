#include "helpers.h"
#include <stdio.h>
#include <string.h>

// Ceiling division: returns the smallest integer >= a/b
int ceildiv(int a, int b) {
    if (b == 0) {
        return 0; // Handle division by zero
    }
    return (a + b - 1) / b;
}

// Floor division: returns the largest integer <= a/b
int floordiv(int a, int b) {
    if (b == 0) {
        return 0; // Handle division by zero
    }
    return a / b;
}

// Get the value of a bit in the bitmap (0 or 1)
int bitmapget(char *bitmap, size_t nbits, size_t index) {
    // Error checking
    if (bitmap == NULL) {
        fprintf(stderr, "bitmapget: bitmap is NULL\n");
        return -1;
    }
    
    if (index >= nbits) {
        fprintf(stderr, "bitmapget: index %zu out of range (max %zu)\n", index, nbits - 1);
        return -1;
    }
    
    // Calculate byte and bit position
    size_t byte_index = index / 8;
    size_t bit_index = index % 8;
    
    // Extract the bit
    return (bitmap[byte_index] >> bit_index) & 1;
}

// Set the value of a bit in the bitmap (0 or 1)
int bitmapset(char *bitmap, size_t nbits, size_t index, bool value) {
    // Error checking
    if (bitmap == NULL) {
        fprintf(stderr, "bitmapset: bitmap is NULL\n");
        return -1;
    }
    
    if (index >= nbits) {
        fprintf(stderr, "bitmapset: index %zu out of range (max %zu)\n", index, nbits - 1);
        return -1;
    }
    
    // Calculate byte and bit position
    size_t byte_index = index / 8;
    size_t bit_index = index % 8;
    
    // Set or clear the bit
    if (value) {
        bitmap[byte_index] |= (1 << bit_index);  // Set bit to 1
    } else {
        bitmap[byte_index] &= ~(1 << bit_index); // Set bit to 0
    }
    
    return 0;
}

// Clear the entire bitmap (set all bits to 0)
int bitmapclear(char *bitmap, size_t nbits) {
    // Error checking
    if (bitmap == NULL) {
        fprintf(stderr, "bitmapclear: bitmap is NULL\n");
        return -1;
    }
    
    if (nbits == 0) {
        fprintf(stderr, "bitmapclear: nbits is 0\n");
        return -1;
    }
    
    // Calculate the number of bytes needed to represent nbits
    size_t bytes_needed = ceildiv(nbits, 8);
    
    // Clear all bytes in the bitmap
    memset(bitmap, 0, bytes_needed);
    
    return 0;
}
