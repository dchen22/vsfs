#ifndef HELPERS_H
#define HELPERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Ceiling division: returns the smallest integer >= a/b
int ceildiv(int a, int b);

// Floor division: returns the largest integer <= a/b
int floordiv(int a, int b);

// Get the value of a bit in the bitmap (0 or 1)
int bitmapget(char *bitmap, size_t nbits, size_t index);

// Set the value of a bit in the bitmap (0 or 1)
int bitmapset(char *bitmap, size_t nbits, size_t index, bool value);

// Clear the entire bitmap (set all bits to 0)
int bitmapclear(char *bitmap, size_t nbits);

// Initialize the entire bitmap (set all bits to 0)
int bitmapinit(char *bitmap, size_t nbits);

// Find the first available bit, set it to 1, and return its index (-1 if full)
int bitmapalloc(char *bitmap, size_t nbits);

#endif // HELPERS_H
