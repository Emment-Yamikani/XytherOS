#include <bits/errno.h>
#include <sys/mman/mman.h>
#include <mm/mem_stats.h>
#include <sys/sysprot.h>
#include <sys/thread.h>
#include <sys/proc.h>

int sys_getpagesize(void) {
    return getpagesize();
}

void *sys_sbrk(intptr_t increment) {
    return sbrk(increment);
}

int sys_getmemstats(mem_stats_t *info) {
    return getmemstats(info);
}

void *sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off) {
    return mmap(addr, len, prot, flags, fd, off);
}

int sys_munmap(void *addr, size_t len) {
    return munmap(addr, len);
}

int sys_mprotect(void *addr, size_t len, int prot) {
    return mprotect(addr, len, prot);
}

void *sys_mremap(void *old_address, size_t old_size, size_t new_size, int flags, ... /* void *new_address */);
int sys_mlock(const void *addr, size_t len);
int sys_mlock2(const void *addr, size_t len, unsigned int flags);
int sys_munlock(const void *addr, size_t len);
int sys_mlockall(int flags);
int sys_munlockall(void);
int sys_msync(void *addr, size_t length, int flags);
int sys_madvise(void *addr, size_t length, int advice);