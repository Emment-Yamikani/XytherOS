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

#ifndef strncmp
    #define strncmp xytherOS_strncmp
#endif


#ifndef strlen
    #define strlen xytherOS_strlen
#endif

bool string_eq(const char *s0, const char *s1);

char *safestrncpy(char *restrict s, const char *restrict t, size_t n);