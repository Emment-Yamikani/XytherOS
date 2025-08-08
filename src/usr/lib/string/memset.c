#include <xyther/string.h>

void *memtset(void *s, int c, size_t n) {
    for (char *S = (char *)s; n--; *S++ = c);
    return s;
}