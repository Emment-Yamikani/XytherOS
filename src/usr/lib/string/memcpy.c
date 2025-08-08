#include <xyther/string.h>

void *memcpy(void * dst, const void *src, size_t n) {
    char *dst_c = (char *)dst, *src_c = (char *)src;

    for (; dst_c && src_c && n; *dst_c++ = *src_c++);

    return dst;
}