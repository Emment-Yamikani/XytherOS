#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h> // SSE intrinsics

// xytherOS_memset: Fill memory with a specific value
void* xytherOS_memset(void* dest, int value, size_t count) {
    uint8_t* p = (uint8_t*)dest;
    uint8_t v = (uint8_t)value;

    // Align to 16-byte boundary for SSE
    while (((uintptr_t)p & 15) && count) {
        *p++ = v;
        count--;
    }

    // Use SSE to set 16-byte chunks
    __m128i val128 = _mm_set1_epi8(v);
    while (count >= 16) {
        _mm_store_si128((__m128i*)p, val128);
        p += 16;
        count -= 16;
    }

    // Set remaining bytes
    while (count--) {
        *p++ = v;
    }

    return dest;
}

void* xytherOS_memsetw(void* dest, uint16_t value, size_t count) {
    uint16_t* p = (uint16_t*)dest;

    // Align to 16-byte boundary
    while (((uintptr_t)p & 15) && count) {
        *p++ = value;
        count--;
    }

    // Use SSE to set 16-bit values in 16-byte chunks (8 values at a time)
    __m128i val128 = _mm_set1_epi16(value);
    while (count >= 8) {
        _mm_store_si128((__m128i*)p, val128);
        p += 8;
        count -= 8;
    }

    // Set remaining words
    while (count--) {
        *p++ = value;
    }

    return dest;
}

// xytherOS_memcpy: Copy memory from source to destination
void* xytherOS_memcpy(void* dest, const void* src, size_t count) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;

    // Align to 16-byte boundary for SSE
    while (((uintptr_t)d & 15) && count) {
        *d++ = *s++;
        count--;
    }

    // Use SSE to copy 16-byte chunks
    while (count >= 16) {
        __m128i val128 = _mm_loadu_si128((__m128i*)s);
        _mm_store_si128((__m128i*)d, val128);
        d += 16;
        s += 16;
        count -= 16;
    }

    // Copy remaining bytes
    while (count--) {
        *d++ = *s++;
    }

    return dest;
}

// xytherOS_memmove: Copy memory with handling for overlapping regions
void* xytherOS_memmove(void* dest, const void* src, size_t count) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;

    if (d < s) {
        // Copy forward
        return xytherOS_memcpy(dest, src, count);
    } else if (d > s) {
        // Copy backward for overlapping regions
        d += count;
        s += count;

        // Align to 16-byte boundary for SSE
        while (((uintptr_t)d & 15) && count) {
            *--d = *--s;
            count--;
        }

        // Use SSE to copy 16-byte chunks
        while (count >= 16) {
            d -= 16;
            s -= 16;
            __m128i val128 = _mm_loadu_si128((__m128i*)s);
            _mm_store_si128((__m128i*)d, val128);
            count -= 16;
        }

        // Copy remaining bytes
        while (count--) {
            *--d = *--s;
        }
    }

    return dest;
}

// xytherOS_memcmp: Compare two memory regions
int xytherOS_memcmp(const void* ptr1, const void* ptr2, size_t count) {
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    // Use SSE to compare 16-byte chunks
    while (count >= 16) {
        __m128i val1 = _mm_loadu_si128((__m128i*)p1);
        __m128i val2 = _mm_loadu_si128((__m128i*)p2);
        __m128i cmp = _mm_cmpeq_epi8(val1, val2);
        int mask = _mm_movemask_epi8(cmp);
        if (mask != 0xFFFF) {
            // Find the first mismatched byte
            for (int i = 0; i < 16; i++) {
                if (p1[i] != p2[i]) {
                    return (p1[i] < p2[i]) ? -1 : 1;
                }
            }
        }
        p1 += 16;
        p2 += 16;
        count -= 16;
    }

    // Compare remaining bytes
    while (count--) {
        if (*p1 != *p2) {
            return (*p1 < *p2) ? -1 : 1;
        }
        p1++;
        p2++;
    }

    return 0;
}

// xytherOS_memchr: Find the first occurrence of a byte in memory
void* xytherOS_memchr(const void* ptr, int value, size_t count) {
    const uint8_t* p = (const uint8_t*)ptr;
    uint8_t v = (uint8_t)value;

    // Use SSE to search for the byte
    __m128i val128 = _mm_set1_epi8(v);
    while (count >= 16) {
        __m128i chunk = _mm_loadu_si128((__m128i*)p);
        __m128i cmp = _mm_cmpeq_epi8(chunk, val128);
        int mask = _mm_movemask_epi8(cmp);
        if (mask) {
            // Find the first matching byte
            for (int i = 0; i < 16; i++) {
                if (p[i] == v) {
                    return (void*)(p + i);
                }
            }
        }
        p += 16;
        count -= 16;
    }

    // Search remaining bytes
    while (count--) {
        if (*p == v) {
            return (void*)p;
        }
        p++;
    }

    return NULL;
}

char* xytherOS_strncpy(char* dest, const char* src, size_t n) {
    char* d = dest;

    while (n && ((uintptr_t)src & 15)) {
        if ((*d++ = *src++) == '\0') {
            while (--n) *d++ = '\0';
            return dest;
        }
        n--;
    }

    // Use SSE to copy 16 bytes at a time
    while (n >= 16) {
        __m128i chunk = _mm_loadu_si128((__m128i*)src);
        _mm_storeu_si128((__m128i*)d, chunk);

        int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, _mm_setzero_si128()));
        if (mask) {
            size_t zero_idx = __builtin_ctz(mask);
            for (size_t i = zero_idx; i < 16 && n; i++, n--) {
                d[i] = '\0';
            }
            return dest;
        }

        d += 16;
        src += 16;
        n -= 16;
    }

    // Copy remaining bytes
    while (n--) {
        if ((*d++ = *src++) == '\0') {
            while (n--) *d++ = '\0';
            return dest;
        }
    }

    return dest;
}

char* xytherOS_strcpy(char* dest, const char* src) {
    char* d = dest;

    while (((uintptr_t)src & 15)) {
        if ((*d++ = *src++) == '\0') return dest;
    }

    // Use SSE to copy 16 bytes at a time
    while (1) {
        __m128i chunk = _mm_loadu_si128((__m128i*)src);
        _mm_storeu_si128((__m128i*)d, chunk);

        int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, _mm_setzero_si128()));
        if (mask) {
            return dest;
        }

        d += 16;
        src += 16;
    }
}

int xytherOS_strcmp(const char* s1, const char* s2) {
    while (((uintptr_t)s1 & 15) && ((uintptr_t)s2 & 15)) {
        if (*s1 != *s2 || *s1 == '\0') {
            return (unsigned char)*s1 - (unsigned char)*s2;
        }
        s1++;
        s2++;
    }

    // Use SSE to compare 16 bytes at a time
    while (1) {
        __m128i chunk1 = _mm_loadu_si128((__m128i*)s1);
        __m128i chunk2 = _mm_loadu_si128((__m128i*)s2);
        __m128i cmp = _mm_cmpeq_epi8(chunk1, chunk2);
        int mask = _mm_movemask_epi8(cmp);

        if (mask != 0xFFFF) {
            for (int i = 0; i < 16; i++) {
                if (s1[i] != s2[i]) {
                    return (unsigned char)s1[i] - (unsigned char)s2[i];
                }
                if (s1[i] == '\0') return 0;
            }
        }

        s1 += 16;
        s2 += 16;
    }
}

int xytherOS_strncmp(const char *s1, const char *s2, size_t n) {
    if (n == 0)
        return 0;

    while (n-- && *s1 == *s2) {
        if (!n || !*s1)
            break;
        s1++;
        s2++;
    }
    return (*(uint8_t *)s1) - (*(uint8_t *)s2);
}

size_t xytherOS_strlen(const char* str) {
    const char* s = str;
    
    // Align to 16-byte boundary
    while ((uintptr_t)s & 15) {
        if (*s == '\0') return s - str;
        s++;
    }

    // Use SSE to process 16 bytes at a time
    while (1) {
        __m128i chunk = _mm_loadu_si128((__m128i*)s);
        __m128i zero = _mm_setzero_si128();
        __m128i cmp = _mm_cmpeq_epi8(chunk, zero);
        int mask = _mm_movemask_epi8(cmp);

        if (mask) {
            return (s - str) + __builtin_ctz(mask);
        }

        s += 16;
    }
}

char* xytherOS_strstr(const char* haystack, const char* needle) {
    size_t needle_len = xytherOS_strlen(needle);
    if (!needle_len) return (char*)haystack;

    while (*haystack) {
        if (xytherOS_memcmp(haystack, needle, needle_len) == 0) {
            return (char*)haystack;
        }
        haystack++;
    }

    return NULL;
}

char* xytherOS_strrchr(const char* str, int c) {
    char ch = (char)c;
    const char* last = NULL;

    while (*str) {
        if (*str == ch) last = str;
        str++;
    }

    return (char*)last;
}

char* xytherOS_strchr(const char* str, int c) {
    char ch = (char)c;
    
    while ((uintptr_t)str & 15) {
        if (*str == ch) return (char*)str;
        if (*str == '\0') return NULL;
        str++;
    }

    // Use SSE to search in 16-byte chunks
    __m128i val = _mm_set1_epi8(ch);
    while (1) {
        __m128i chunk = _mm_loadu_si128((__m128i*)str);
        __m128i cmp = _mm_cmpeq_epi8(chunk, val);
        int mask = _mm_movemask_epi8(cmp);

        if (mask) {
            return (char*)str + __builtin_ctz(mask);
        }

        cmp = _mm_cmpeq_epi8(chunk, _mm_setzero_si128());
        mask = _mm_movemask_epi8(cmp);
        if (mask) return NULL;

        str += 16;
    }
}

bool string_eq(const char *s0, const char *s1) {
    if (xytherOS_strlen(s0) != xytherOS_strlen(s1))
        return false;
    return !xytherOS_strcmp(s0, s1);
}

// Like strncpy but guaranteed to NUL-terminate.
char *safestrncpy(char *restrict s, const char *restrict t, size_t n) {
    char *os;

    os = s;
    if (n <= 0)
        return os;
    while (--n > 0 && (*s++ = *t++) != 0)
        ;
    *s = 0;
    return os;
}