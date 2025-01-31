#pragma once

#include <stddef.h>

// xytherOS_memset: Fill memory with a specific value
void* xytherOS_memset(void* dest, int value, size_t count);
// xytherOS_memcpy: Copy memory from source to destination
void* xytherOS_memcpy(void* dest, const void* src, size_t count);
// xytherOS_memmove: Copy memory with handling for overlapping regions
void* xytherOS_memmove(void* dest, const void* src, size_t count);
// xytherOS_memcmp: Compare two memory regions
int xytherOS_memcmp(const void* ptr1, const void* ptr2, size_t count);
// xytherOS_memchr: Find the first occurrence of a byte in memory
void* xytherOS_memchr(const void* ptr, int value, size_t count);