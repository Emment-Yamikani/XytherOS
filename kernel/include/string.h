#pragma once

#include <xyther_string.h>

#ifndef memset
    #define memset xytherOS_memset
#endif

#ifndef memcpy
    #define memcpy xytherOS_memcpy
#endif

#ifndef memmove
    #define memmove xytherOS_memmove
#endif

#ifndef memcmp
    #define memcmp xytherOS_memcmp
#endif

#ifndef memchr
    #define memchr xytherOS_memchr
#endif

#ifndef memsetw
    #define memsetw xytherOS_memsetw
#endif

#ifndef strstr
    #define strstr xytherOS_strstr
#endif

#ifndef strrchr
    #define strrchr xytherOS_strrchr
#endif

#ifndef strncpy
    #define strncpy xytherOS_strncpy
#endif

#ifndef strcpy
    #define strcpy xytherOS_strcpy
#endif

#ifndef strcmp
    #define strcmp xytherOS_strcmp
#endif

#ifndef strlen
    #define strlen xytherOS_strlen
#endif

#ifndef safestrncpy
#define safestrncpy xytherOS_safestrncpy
#endif

#ifndef lfind
#define lfind xytherOS_lfind
#endif

#ifndef rfind
#define rfind xytherOS_rfind
#endif

#ifndef strtok_r
#define strtok_r xytherOS_strtok_r
#endif

#ifndef strtok
#define strtok xytherOS_strtok
#endif

#ifndef strcat
#define strcat xytherOS_strcat
#endif

#ifndef strncat
#define strncat xytherOS_strncat
#endif

#ifndef strncmp
#define strncmp xytherOS_strncmp
#endif

#ifndef strcasecmp
#define strcasecmp xytherOS_strcasecmp
#endif

#ifndef strncasecmp
#define strncasecmp xytherOS_strncasecmp
#endif

#ifndef strdup
#define strdup xytherOS_strdup
#endif

extern bool string_eq(const char *s0, const char *s1);
extern int tokenize(char *s, int delim, size_t *ntoks, char ***ptokenized, char **plast_tok);
extern void tokens_free(char **tokens);
extern int canonicalize_path(const char *path, size_t *ntoks, char ***ptokenized, char **plast);
extern char *combine_strings(const char *s0, const char *s1);