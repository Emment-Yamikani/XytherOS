#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * xytherOS_memset - Fill memory with a specific value.
 * @dest: Pointer to the memory area to fill.
 * @value: Byte value to set.
 * @count: Number of bytes to set.
 *
 * Returns: Pointer to the memory area @dest.
 */
extern void *xytherOS_memset(void *dest, int value, size_t count);

/**
 * xytherOS_memcpy - Copy memory from source to destination.
 * @dest: Pointer to the destination memory area.
 * @src: Pointer to the source memory area.
 * @count: Number of bytes to copy.
 *
 * Returns: Pointer to the destination memory area @dest.
 */
extern void *xytherOS_memcpy(void *dest, const void *src, size_t count);

/**
 * xytherOS_memmove - Copy memory with handling for overlapping regions.
 * @dest: Pointer to the destination memory area.
 * @src: Pointer to the source memory area.
 * @count: Number of bytes to copy.
 *
 * Returns: Pointer to the destination memory area @dest.
 */
extern void *xytherOS_memmove(void *dest, const void *src, size_t count);

/**
 * xytherOS_memcmp - Compare two memory regions.
 * @ptr1: Pointer to the first memory block.
 * @ptr2: Pointer to the second memory block.
 * @count: Number of bytes to compare.
 *
 * Returns: 0 if equal, negative if @ptr1 < @ptr2, positive if @ptr1 > @ptr2.
 */
extern int xytherOS_memcmp(const void *ptr1, const void *ptr2, size_t count);

/**
 * xytherOS_memchr - Find the first occurrence of a byte in memory.
 * @ptr: Pointer to the memory block.
 * @value: Byte value to search for.
 * @count: Number of bytes to search.
 *
 * Returns: Pointer to the first occurrence of @value, or NULL if not found.
 */
extern void *xytherOS_memchr(const void *ptr, int value, size_t count);

/**
 * xytherOS_memsetw - Fill memory with a specific 16-bit value.
 * @dest: Pointer to the memory block to fill.
 * @value: 16-bit value to set.
 * @count: Number of 16-bit words to set.
 *
 * Returns: Pointer to the memory area @dest.
 */
extern void *xytherOS_memsetw(void *dest, uint16_t value, size_t count);

/**
 * xytherOS_strstr - Locate a substring.
 * @haystack: Pointer to the main string.
 * @needle: Pointer to the substring.
 *
 * Returns: Pointer to the first occurrence of @needle, or NULL if not found.
 */
extern char *xytherOS_strstr(const char *haystack, const char *needle);

/**
 * xytherOS_strrchr - Locate the last occurrence of a character in a string.
 * @str: Pointer to the input string.
 * @c: Character to locate.
 *
 * Returns: Pointer to the last occurrence of @c, or NULL if not found.
 */
extern char *xytherOS_strrchr(const char *str, int c);

/**
 * xytherOS_strncpy - Copy a string up to a specified length.
 * @dest: Pointer to the destination buffer.
 * @src: Pointer to the source string.
 * @n: Maximum number of characters to copy.
 *
 * Returns: Pointer to the destination buffer @dest.
 */
extern char *xytherOS_strncpy(char *dest, const char *src, size_t n);

/**
 * xytherOS_strcpy - Copy a null-terminated string.
 * @dest: Pointer to the destination buffer.
 * @src: Pointer to the source string.
 *
 * Returns: Pointer to the destination buffer @dest.
 */
extern char *xytherOS_strcpy(char *dest, const char *src);

/**
 * xytherOS_strcmp - Compare two null-terminated strings.
 * @s1: Pointer to the first string.
 * @s2: Pointer to the second string.
 *
 * Returns: 0 if equal, negative if @s1 < @s2, positive if @s1 > @s2.
 */
extern int xytherOS_strcmp(const char *s1, const char *s2);

/**
 * xytherOS_strlen - Compute the length of a null-terminated string.
 * @str: Pointer to the input string.
 *
 * Returns: Length of the string excluding the null terminator.
 */
extern size_t xytherOS_strlen(const char *str);

extern int xytherOS_strncmp(const char *s1, const char *s2, size_t n);

extern char *xytherOS_safestrncpy(char *dest, const char *src, size_t n);
extern char *xytherOS_lfind(const char *str, int ch);
extern char *xytherOS_rfind(const char *str, int ch);
extern char *xytherOS_strtok_r(char *s, const char *delim, char **saveptr);
extern char *xytherOS_strtok(char *s, const char *delim);
extern char *xytherOS_strcat(char *dest, const char *src);
extern char *xytherOS_strncat(char *dest, const char *src, size_t n);
extern int  xytherOS_strncmp(const char *s1, const char *s2, size_t n);
extern int  xytherOS_strcasecmp(const char *s1, const char *s2);
extern int  xytherOS_strncasecmp(const char *s1, const char *s2, size_t n);
extern char *xytherOS_strdup(const char *s);