#include "helpers.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

// Find the first available bit, set it to 1, and return its index
uint32_t bitmapalloc(char *bitmap, size_t nbits) {
    // Error checking
    if (bitmap == NULL) {
        fprintf(stderr, "bitmapalloc: bitmap is NULL\n");
        return 0;
    }
    
    if (nbits == 0) {
        fprintf(stderr, "bitmapalloc: nbits is 0\n");
        return 0;
    }
    
    // Search for the first free bit (0), starting from index 1 (skip first bit)
    for (size_t i = 1; i < nbits; i++) {
        int bit_value = bitmapget(bitmap, nbits, i);
        if (bit_value < 0) {
            fprintf(stderr, "bitmapalloc: error reading bit %zu\n", i);
            return 0;
        }
        
        if (bit_value == 0) {
            // Found a free bit, set it to 1
            if (bitmapset(bitmap, nbits, i, true) < 0) {
                fprintf(stderr, "bitmapalloc: error setting bit %zu\n", i);
                return 0;
            }
            return (uint32_t)i;  // Return the index of the allocated bit
        }
    }
    
    // No free bits found
    return 0;
}


void free_split(char **parts)
{
    if (!parts) return;
    for (size_t i = 0; parts[i]; ++i) free(parts[i]);
    free(parts);
}

char **split_path(const char *path)
/* Returns a NULL-terminated array of malloc'd strings.
   Caller must free with free_split(parts). On error, returns NULL. */
{
    if (!path) return NULL;

    // 1) Count segments
    size_t n = 0;
    const char *p = path;
    while (*p) {
        while (*p == '/') ++p;                   // skip slashes
        if (!*p) break;
        while (*p && *p != '/') ++p;             // consume segment
        ++n;
    }

    // Allocate array of pointers (+1 for NULL terminator)
    char **parts = malloc((n + 1) * sizeof *parts);
    if (!parts) return NULL;

    // 2) Fill segments
    p = path;
    size_t i = 0;
    while (*p) {
        while (*p == '/') ++p;
        if (!*p) break;
        const char *start = p;
        while (*p && *p != '/') ++p;
        size_t len = (size_t)(p - start);

        char *seg = malloc(len + 1);
        if (!seg) {
            free_split(parts);
            return NULL;
        }
        memcpy(seg, start, len);
        seg[len] = '\0';
        parts[i++] = seg;
    }
    parts[i] = NULL;
    return parts;
}

