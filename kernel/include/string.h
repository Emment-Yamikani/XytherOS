#pragma once

#include <xyther_string.h>

extern int memcmp(const void *vl, const void *vr, size_t n);
extern void *memchr(const void *src, int c, size_t n);
extern void *memrchr(const void *m, int c, size_t n);
extern void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size);
extern void *memcpy32(void *restrict dstptr, const void *restrict srcptr, size_t n);
extern void *memset(void *bufptr, int value, size_t size);
extern void *memsetw(void *bufptr, int value, size_t size);
extern int strcmp(const char *l, const char *r);
extern int strcoll(const char *s1, const char *s2);
extern size_t strlen(const char *s);
extern char *stpcpy(char *restrict d, const char *restrict s);
extern char *strcpy(char *restrict dest, const char *restrict src);
extern size_t strspn(const char *s, const char *c);
extern char *strchrnul(const char *s, int c);
extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);
extern size_t strcspn(const char *s, const char *c);
extern char *strpbrk(const char *s, const char *b);
extern char *strstr(const char *h, const char *n);
extern size_t lfind(const char *str, const char accept);
extern size_t rfind(const char *str, const char accept);
extern char *strtok_r(char *str, const char *delim, char **saveptr);
extern char *strtok(char *str, const char *delim);
extern char *strcat(char *restrict dest, const char *restrict src);
extern char *strncat(char *dest, const char *src, size_t n);
extern char *strncpy(char *dest, const char *src, size_t n);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern void *memmove(void *dest, const void *src, size_t n);
extern int strcasecmp(const char *s1, const char *s2);
extern int strncasecmp(const char *s1, const char *s2, size_t n);
extern char *safestrncpy(char *restrict s, const char *restrict t, size_t n);
extern int compare_strings(const char *s0, const char *s1);
extern char *strdup(const char *s);

extern bool string_eq(const char *str0, const char *str1);
extern int tokenize(char *s, int c, size_t *ntoks, char ***ptokenized, char **plast_tok);
extern void tokens_free(char **tokens);
extern int canonicalize_path(const char *path, size_t *ntoks, char ***ptokenized, char **plast);
extern char *combine_strings(const char *s0, const char *s1);