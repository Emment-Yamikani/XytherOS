#include <mm/mmap.h>
#include <bits/errno.h>
#include <sys/thread.h>

void *sbrk(intptr_t increment) {
    (void)increment;
    return (void *)-ENOSYS;
}