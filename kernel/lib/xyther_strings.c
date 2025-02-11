#include <bits/errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <immintrin.h> // SSE intrinsics
#include <mm/kalloc.h>
#include <string.h>

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

/*===========================================================================
  Helper functions for SSE loads and stores.
  These check pointer alignment and use aligned loads/stores when possible.
  ===========================================================================*/
static inline __m128i sse_load(const void *p) {
    return (((uintptr_t)p & 0xF) == 0) ? _mm_load_si128((const __m128i *)p)
                                       : _mm_loadu_si128((const __m128i *)p);
}

static inline void sse_store(void *p, __m128i v) {
    if (((uintptr_t)p & 0xF) == 0)
        _mm_store_si128((__m128i *)p, v);
    else
        _mm_storeu_si128((__m128i *)p, v);
}

/*===========================================================================
  xytherOS_safestrncpy
  Copies at most n-1 characters from src to dest and always NUL-terminates.
  Uses SSE to copy 16 bytes at a time until a NUL is encountered.
  ===========================================================================*/
char *xytherOS_safestrncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    if (n == 0)
        return dest;
    __m128i zero = _mm_setzero_si128();
    while (n - i >= 16) {
        __m128i chunk = sse_load(src + i);
        int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, zero));
        if (mask) {
            int pos = __builtin_ctz(mask);
            memcpy(dest + i, src + i, pos);
            i += pos;
            goto finish;
        }
        sse_store(dest + i, chunk);
        i += 16;
    }
    for (; i < n - 1; i++) {
        dest[i] = src[i];
        if (src[i] == '\0')
            return dest;
    }
finish:
    dest[n - 1] = '\0';
    return dest;
}

/*===========================================================================
  xytherOS_lfind
  Finds (from the left) the first occurrence of character ch in str.
  ===========================================================================*/
char *xytherOS_lfind(const char *str, int ch) {
    __m128i target = _mm_set1_epi8((char)ch);
    __m128i zero = _mm_setzero_si128();
    for (;; str += 16) {
        __m128i block = sse_load(str);
        int eqMask = _mm_movemask_epi8(_mm_cmpeq_epi8(block, target));
        int zMask = _mm_movemask_epi8(_mm_cmpeq_epi8(block, zero));
        if (eqMask)
            return (char *)(str + __builtin_ctz(eqMask));
        if (zMask)
            break;
    }
    return NULL;
}

/*===========================================================================
  xytherOS_rfind
  Finds (from the right) the last occurrence of character ch in str.
  ===========================================================================*/
char *xytherOS_rfind(const char *str, int ch) {
    size_t len = xytherOS_strlen(str);
    /* If searching for the NUL terminator, return the end */
    if (ch == 0)
        return (char *)(str + len);
    const char *last = NULL;
    for (size_t i = 0; i < len; i++) {
        if (str[i] == (char)ch)
            last = str + i;
    }
    return (char *)last;
}

/*===========================================================================
  Helpers for xytherOS_strtok_r:
  These routines use SSE to quickly skip over or find delimiter characters.
  ===========================================================================*/
static const char *sse_skip_delim(const char *s, const char *delim, int dlen) {
    __m128i zero = _mm_setzero_si128();
    for (;;) {
        __m128i block = sse_load(s);
        int nullMask = _mm_movemask_epi8(_mm_cmpeq_epi8(block, zero));
        int dmask = 0;
        for (int i = 0; i < dlen; i++) {
            __m128i d = _mm_set1_epi8(delim[i]);
            dmask |= _mm_movemask_epi8(_mm_cmpeq_epi8(block, d));
        }
        if (nullMask) {
            int valid = __builtin_ctz(nullMask);
            if ((dmask & ((1 << valid) - 1)) == ((1 << valid) - 1))
                return s + valid;
            else
                return s + __builtin_ctz(~(dmask & ((1 << valid) - 1)));
        }
        if (dmask == 0xFFFF)
            s += 16;
        else
            return s + __builtin_ctz(~dmask);
    }
}

static const char *sse_find_delim(const char *s, const char *delim, int dlen) {
    __m128i zero = _mm_setzero_si128();
    for (;;) {
        __m128i block = sse_load(s);
        int nullMask = _mm_movemask_epi8(_mm_cmpeq_epi8(block, zero));
        int dmask = 0;
        for (int i = 0; i < dlen; i++) {
            __m128i d = _mm_set1_epi8(delim[i]);
            dmask |= _mm_movemask_epi8(_mm_cmpeq_epi8(block, d));
        }
        if (nullMask) {
            int valid = __builtin_ctz(nullMask);
            int mask = dmask & ((1 << valid) - 1);
            if (mask)
                return s + __builtin_ctz(mask);
            else
                return s + valid;
        }
        if (dmask)
            return s + __builtin_ctz(dmask);
        s += 16;
    }
}

/*===========================================================================
  xytherOS_strtok_r
  A reentrant string tokenizer that uses SSE routines when the delimiter
  set is short (<= 16 characters). Otherwise, it falls back to a scalar loop.
  ===========================================================================*/
char *xytherOS_strtok_r(char *s, const char *delim, char **saveptr) {
    if (!delim || !delim[0]) {  /* Empty delimiter returns the whole string */
        *saveptr = s ? s + strlen(s) : NULL;
        return s;
    }
    if (!s)
        s = *saveptr;
    int dlen = (int)strlen(delim);
    if (dlen > 0 && dlen <= 16) {
        s = (char *)sse_skip_delim(s, delim, dlen);
        if (!*s) {
            *saveptr = s;
            return NULL;
        }
        char *token = s;
        char *end = (char *)sse_find_delim(s, delim, dlen);
        if (*end) {
            *end = '\0';
            *saveptr = end + 1;
        } else
            *saveptr = end;
        return token;
    } else {
        /* Fallback scalar code if delimiter set is large */
        while (*s && xytherOS_strchr(delim, *s))
            s++;
        if (!*s) {
            *saveptr = s;
            return NULL;
        }
        char *token = s;
        while (*s && !xytherOS_strchr(delim, *s))
            s++;
        if (*s) {
            *s = '\0';
            *saveptr = s + 1;
        } else
            *saveptr = s;
        return token;
    }
}

char *xytherOS_strtok(char *s, const char *delim) {
    static char *last;
    return xytherOS_strtok_r(s, delim, &last);
}

/*===========================================================================
  xytherOS_strcat
  Concatenates src onto the end of dest.
  Locates the end of dest using SSE and then copies src (using SSE) until NUL.
  ===========================================================================*/
char *xytherOS_strcat(char *dest, const char *src) {
    char *d = dest;
    __m128i zero = _mm_setzero_si128();
    for (;;) {
        __m128i block = sse_load(d);
        int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(block, zero));
        if (mask) {
            d += __builtin_ctz(mask);
            break;
        }
        d += 16;
    }
    char *ret = dest;
    for (;;) {
        __m128i chunk = sse_load(src);
        int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, zero));
        if (mask) {
            int pos = __builtin_ctz(mask);
            memcpy(d, src, pos);
            d += pos;
            *d = '\0';
            break;
        }
        sse_store(d, chunk);
        d += 16;
        src += 16;
    }
    return ret;
}

/*===========================================================================
  xytherOS_strncat
  Appends at most n characters from src to dest and always NUL-terminates.
  ===========================================================================*/
char *xytherOS_strncat(char *dest, const char *src, size_t n) {
    char *d = dest;
    __m128i zero = _mm_setzero_si128();
    for (;;) {
        __m128i block = sse_load(d);
        int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(block, zero));
        if (mask) {
            d += __builtin_ctz(mask);
            break;
        }
        d += 16;
    }
    char *ret = dest;
    size_t copied = 0;
    while (copied < n) {
        __m128i chunk = sse_load(src);
        int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, zero));
        if (mask) {
            int pos = __builtin_ctz(mask);
            size_t to_copy = (n - copied < (size_t)pos) ? (n - copied) : (size_t)pos;
            memcpy(d, src, to_copy);
            d += to_copy;
            copied += to_copy;
            break;
        }
        if (n - copied >= 16) {
            sse_store(d, chunk);
            d += 16;
            src += 16;
            copied += 16;
        } else {
            for (size_t i = 0; i < n - copied; i++) {
                if (src[i] == '\0') {
                    memcpy(d, src, i);
                    copied += i;
                    d += i;
                    goto finish_ncat;
                }
            }
            memcpy(d, src, n - copied);
            d += (n - copied);
            copied = n;
            break;
        }
    }
finish_ncat:
    *d = '\0';
    return ret;
}

/*===========================================================================
  xytherOS_strncmp
  Compares at most n characters of s1 and s2.
  Uses SSE to compare 16 bytes at a time.
  ===========================================================================*/
int xytherOS_strncmp(const char *s1, const char *s2, size_t n) {
    __m128i zero = _mm_setzero_si128();
    while (n >= 16) {
        __m128i v1 = sse_load(s1);
        __m128i v2 = sse_load(s2);
        __m128i cmp = _mm_cmpeq_epi8(v1, v2);
        int mask = _mm_movemask_epi8(cmp);
        if (mask != 0xFFFF) {
            int diff = __builtin_ctz(~mask);
            return ((unsigned char *)s1)[diff] - ((unsigned char *)s2)[diff];
        }
        int zmask = _mm_movemask_epi8(_mm_cmpeq_epi8(v1, zero));
        if (zmask) {
            for (int i = 0; i < 16 && n; i++) {
                if (s1[i] != s2[i])
                    return ((unsigned char *)s1)[i] - ((unsigned char *)s2)[i];
                if (s1[i] == '\0')
                    return 0;
                n--;
            }
            return 0;
        }
        s1 += 16; s2 += 16; n -= 16;
    }
    while (n--) {
        unsigned char c1 = (unsigned char)*s1++;
        unsigned char c2 = (unsigned char)*s2++;
        if (c1 != c2)
            return c1 - c2;
        if (c1 == '\0')
            return 0;
    }
    return 0;
}

/*===========================================================================
  xytherOS_strcasecmp
  Case-insensitive string compare.
  Uses SSE to convert A-Z to lower-case (by OR-ing with 0x20) before comparing.
  ===========================================================================*/
int xytherOS_strcasecmp(const char *s1, const char *s2) {
    __m128i zero = _mm_setzero_si128();
    __m128i offset = _mm_set1_epi8(0x20);
    __m128i A = _mm_set1_epi8('A' - 1);
    __m128i Z = _mm_set1_epi8('Z' + 1);
    for (;;) {
        __m128i v1 = sse_load(s1);
        __m128i v2 = sse_load(s2);
        __m128i mask1 = _mm_and_si128(_mm_cmpgt_epi8(v1, A), _mm_cmplt_epi8(v1, Z));
        __m128i lower1 = _mm_or_si128(v1, _mm_and_si128(mask1, offset));
        __m128i mask2 = _mm_and_si128(_mm_cmpgt_epi8(v2, A), _mm_cmplt_epi8(v2, Z));
        __m128i lower2 = _mm_or_si128(v2, _mm_and_si128(mask2, offset));
        __m128i cmp = _mm_cmpeq_epi8(lower1, lower2);
        int mask = _mm_movemask_epi8(cmp);
        if (mask != 0xFFFF) {
            int diff = __builtin_ctz(~mask);
            unsigned char c1 = ((unsigned char *)&lower1)[diff];
            unsigned char c2 = ((unsigned char *)&lower2)[diff];
            return c1 - c2;
        }
        int zmask = _mm_movemask_epi8(_mm_cmpeq_epi8(lower1, zero));
        if (zmask) {
            for (int i = 0; i < 16; i++) {
                unsigned char c1 = s1[i], c2 = s2[i];
                if (c1 >= 'A' && c1 <= 'Z')
                    c1 |= 0x20;
                if (c2 >= 'A' && c2 <= 'Z')
                    c2 |= 0x20;
                if (c1 != c2)
                    return c1 - c2;
                if (c1 == '\0')
                    return 0;
            }
        }
        s1 += 16; s2 += 16;
    }
}

/*===========================================================================
  xytherOS_strncasecmp
  As above but comparing at most n characters.
  ===========================================================================*/
int xytherOS_strncasecmp(const char *s1, const char *s2, size_t n) {
    if (n == 0)
        return 0;
    __m128i zero = _mm_setzero_si128();
    __m128i offset = _mm_set1_epi8(0x20);
    __m128i A = _mm_set1_epi8('A' - 1);
    __m128i Z = _mm_set1_epi8('Z' + 1);
    while (n >= 16) {
        __m128i v1 = sse_load(s1);
        __m128i v2 = sse_load(s2);
        __m128i mask1 = _mm_and_si128(_mm_cmpgt_epi8(v1, A), _mm_cmplt_epi8(v1, Z));
        __m128i lower1 = _mm_or_si128(v1, _mm_and_si128(mask1, offset));
        __m128i mask2 = _mm_and_si128(_mm_cmpgt_epi8(v2, A), _mm_cmplt_epi8(v2, Z));
        __m128i lower2 = _mm_or_si128(v2, _mm_and_si128(mask2, offset));
        __m128i cmp = _mm_cmpeq_epi8(lower1, lower2);
        int mask = _mm_movemask_epi8(cmp);
        if (mask != 0xFFFF) {
            int diff = __builtin_ctz(~mask);
            unsigned char c1 = ((unsigned char *)&lower1)[diff];
            unsigned char c2 = ((unsigned char *)&lower2)[diff];
            return c1 - c2;
        }
        int zmask = _mm_movemask_epi8(_mm_cmpeq_epi8(lower1, zero));
        if (zmask) {
            for (int i = 0; i < 16 && n; i++) {
                unsigned char c1 = s1[i], c2 = s2[i];
                if (c1 >= 'A' && c1 <= 'Z')
                    c1 |= 0x20;
                if (c2 >= 'A' && c2 <= 'Z')
                    c2 |= 0x20;
                if (c1 != c2)
                    return c1 - c2;
                if (c1 == '\0')
                    return 0;
                n--;
            }
            return 0;
        }
        s1 += 16; s2 += 16; n -= 16;
    }
    while (n--) {
        unsigned char c1 = (unsigned char)*s1++;
        unsigned char c2 = (unsigned char)*s2++;
        if (c1 >= 'A' && c1 <= 'Z')
            c1 |= 0x20;
        if (c2 >= 'A' && c2 <= 'Z')
            c2 |= 0x20;
        if (c1 != c2)
            return c1 - c2;
        if (c1 == '\0')
            return 0;
    }
    return 0;
}

/*===========================================================================
  xytherOS_strdup
  Duplicates the string s (allocating memory) using our SSE-optimized copy.
  ===========================================================================*/
char *xytherOS_strdup(const char *s) {
    size_t len = xytherOS_strlen(s);
    char *dup = (char *)kmalloc(len + 1);
    if (!dup)
        return NULL;
    const char *src = s;
    char *dst = dup;
    __m128i zero = _mm_setzero_si128();
    for (;;) {
        __m128i chunk = sse_load(src);
        int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, zero));
        if (mask) {
            int pos = __builtin_ctz(mask);
            memcpy(dst, src, pos);
            dst += pos;
            *dst = '\0';
            break;
        }
        sse_store(dst, chunk);
        src += 16;
        dst += 16;
    }
    return dup;
}

int tokenize(char *s, int delim, size_t *ntoks, char ***ptokenized, char **plast_tok) {
    if (!s || !delim || ptokenized == NULL)
        return -EINVAL;

    /* Duplicate the input string */
    size_t len = strlen(s);
    char *buf = kmalloc(len + 1);
    if (!buf)
        return -ENOMEM;
    memcpy(buf, s, len + 1);

    /* Trim leading delimiters */
    char *p = buf;
    while (*p == delim)
        p++;

    if (p != buf) {
        memmove(buf, p, strlen(p) + 1);
        p = buf;
    }

    /* Trim trailing delimiters */
    char *end = buf + len - 1;
    while (end >= p && *end == delim)
        *end-- = '\0';

    /* Estimate the number of tokens */
    size_t count = 0;
    for (char *scan = p; *scan; scan++)
        if (*scan == delim && *(scan + 1) && *(scan + 1) != delim)
            count++;

    /* Allocate tokens array (worst-case count + 1) */
    char **tokens = kmalloc((count + 2) * sizeof(char *));
    if (!tokens) {
        kfree(buf);
        return -ENOMEM;
    }

    /* Extract tokens in-place */
    size_t token_index = 0;
    while (*p) {
        tokens[token_index++] = p;
        while (*p && *p != delim)
            p++;
        if (*p) {
            *p = '\0';
            p++;
            while (*p == delim)
                p++;
        }
    }
    tokens[token_index] = NULL;

    if (ntoks)
        *ntoks = token_index;
    if (plast_tok)
        *plast_tok = token_index ? tokens[token_index - 1] : NULL;
    *ptokenized = tokens;

    return 0;
}

void tokens_free(char **tokens) {
    if (!tokens)
        return;
    kfree(tokens[0]);
    kfree(tokens);
}

int canonicalize_path(const char *path, size_t *ntoks, char ***ptokenized, char **plast) {
    return tokenize((char *)path, '/', ntoks, ptokenized, plast);
}

char *combine_strings(const char *s0, const char *s1) {
    if (!s0 || !s1)
        return NULL;

    size_t len0 = strlen(s0);
    size_t len1 = strlen(s1);
    size_t total_len = len0 + len1 + 1;

    char *result = kmalloc(total_len);
    if (!result)
        return NULL;
    memcpy(result, s0, len0);
    memcpy(result + len0, s1, len1 + 1);  /* include the null terminator */
    return result;
}
