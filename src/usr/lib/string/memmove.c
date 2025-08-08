#include <xyther/string.h>

void *memmove(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;

    // If src < dst and regions overlap, copy backwards
    if (s < d && d < s + n) {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    } else {
        // Safe to copy forward
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    }

    return dst;
}