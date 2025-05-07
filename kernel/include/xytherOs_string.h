#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * xytherOs_memset - Fill memory with a specific value.
 * @dest: Pointer to the memory area to fill.
 * @value: Byte value to set.
 * @count: Number of bytes to set.
 *
 * Returns: Pointer to the memory area @dest.
 */
extern void *xytherOs_memset(void *dest, int value, size_t count);

/**
 * xytherOs_memcpy - Copy memory from source to destination.
 * @dest: Pointer to the destination memory area.
 * @src: Pointer to the source memory area.
 * @count: Number of bytes to copy.
 *
 * Returns: Pointer to the destination memory area @dest.
 */
extern void *xytherOs_memcpy(void *dest, const void *src, size_t count);

/**
 * xytherOs_memmove - Copy memory with handling for overlapping regions.
 * @dest: Pointer to the destination memory area.
 * @src: Pointer to the source memory area.
 * @count: Number of bytes to copy.
 *
 * Returns: Pointer to the destination memory area @dest.
 */
extern void *xytherOs_memmove(void *dest, const void *src, size_t count);

/**
 * xytherOs_memcmp - Compare two memory regions.
 * @ptr1: Pointer to the first memory block.
 * @ptr2: Pointer to the second memory block.
 * @count: Number of bytes to compare.
 *
 * Returns: 0 if equal, negative if @ptr1 < @ptr2, positive if @ptr1 > @ptr2.
 */
extern int xytherOs_memcmp(const void *ptr1, const void *ptr2, size_t count);

/**
 * xytherOs_memchr - Find the first occurrence of a byte in memory.
 * @ptr: Pointer to the memory block.
 * @value: Byte value to search for.
 * @count: Number of bytes to search.
 *
 * Returns: Pointer to the first occurrence of @value, or NULL if not found.
 */
extern void *xytherOs_memchr(const void *ptr, int value, size_t count);

/**
 * xytherOs_memsetw - Fill memory with a specific 16-bit value.
 * @dest: Pointer to the memory block to fill.
 * @value: 16-bit value to set.
 * @count: Number of 16-bit words to set.
 *
 * Returns: Pointer to the memory area @dest.
 */
extern void *xytherOs_memsetw(void *dest, uint16_t value, size_t count);

/**
 * xytherOs_strstr - Locate a substring.
 * @haystack: Pointer to the main string.
 * @needle: Pointer to the substring.
 *
 * Returns: Pointer to the first occurrence of @needle, or NULL if not found.
 */
extern char *xytherOs_strstr(const char *haystack, const char *needle);

/**
 * xytherOs_strrchr - Locate the last occurrence of a character in a string.
 * @str: Pointer to the input string.
 * @c: Character to locate.
 *
 * Returns: Pointer to the last occurrence of @c, or NULL if not found.
 */
extern char *xytherOs_strrchr(const char *str, int c);

/**
 * xytherOs_strncpy - Copy a string up to a specified length.
 * @dest: Pointer to the destination buffer.
 * @src: Pointer to the source string.
 * @n: Maximum number of characters to copy.
 *
 * Returns: Pointer to the destination buffer @dest.
 */
extern char *xytherOs_strncpy(char *dest, const char *src, size_t n);

/**
 * xytherOs_strcpy - Copy a null-terminated string.
 * @dest: Pointer to the destination buffer.
 * @src: Pointer to the source string.
 *
 * Returns: Pointer to the destination buffer @dest.
 */
extern char *xytherOs_strcpy(char *dest, const char *src);

/**
 * xytherOs_strcmp - Compare two null-terminated strings.
 * @s1: Pointer to the first string.
 * @s2: Pointer to the second string.
 *
 * Returns: 0 if equal, negative if @s1 < @s2, positive if @s1 > @s2.
 */
extern int xytherOs_strcmp(const char *s1, const char *s2);

/**
 * xytherOs_strlen - Compute the length of a null-terminated string.
 * @str: Pointer to the input string.
 *
 * Returns: Length of the string excluding the null terminator.
 */
extern size_t xytherOs_strlen(const char *str);

extern int xytherOs_strncmp(const char *s1, const char *s2, size_t n);

extern char *xytherOs_safestrncpy(char *dest, const char *src, size_t n);
extern char *xytherOs_lfind(const char *str, int ch);
extern char *xytherOs_rfind(const char *str, int ch);
extern char *xytherOs_strtok_r(char *s, const char *delim, char **saveptr);
extern char *xytherOs_strtok(char *s, const char *delim);
extern char *xytherOs_strcat(char *dest, const char *src);
extern char *xytherOs_strncat(char *dest, const char *src, size_t n);
extern int  xytherOs_strncmp(const char *s1, const char *s2, size_t n);
extern int  xytherOs_strcasecmp(const char *s1, const char *s2);
extern int  xytherOs_strncasecmp(const char *s1, const char *s2, size_t n);
extern char *xytherOs_strdup(const char *s);

extern bool xytherOs_string_eq(const char *s0, const char *s1);
extern int xytherOs_tokenize(char *s, int delim, size_t *ntoks, char ***ptokenized, char **plast_tok);
extern int xytherOs_tokenize_r(char *s, const char *delim, size_t *ntoks, char ***ptokenized, char **plast_tok);
extern void xytherOs_tokens_free(char **tokens);
extern int xytherOs_canonicalize_path(const char *path, size_t *ntoks, char ***ptokenized, char **plast);
extern char *xytherOs_combine_strings(const char *s0, const char *s1);

#include <stdarg.h>
extern int xytherOs_vsscanf(const char *str, const char *fmt, va_list ap);
extern int xytherOs_sscanf(const char *str, const char *fmt, ...);

extern int __xytherOs_atoi(const char *str);
extern char *__xytherOs_strchr(const char *s, int c);
extern int __xytherOs_tokenize_r(char *s, const char *delim, char **tokens, int max_tokens);
extern char *__xytherOs_strtok_r(char *str, const char *delim, char **saveptr);
extern size_t __xytherOs_strspn(const char *s, const char *accept);
extern size_t __xytherOs_strcspn(const char *s, const char *reject);
extern void __xytherOs_tokens_free(char **tokens);