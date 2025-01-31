#include <stddef.h>
#include <stdint.h>
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