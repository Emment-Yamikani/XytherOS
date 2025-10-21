#pragma once

#include <sys/thread.h>

typedef struct binary_loader {
    int (*check)(inode_t *binary);
    int (*load)(inode_t *binary, mmap_t *mmap);
} binary_loader_t;

#define BINARY_LOADER(name, checker, loader)     \
binary_loader_t __used_section(.__binary_loader) \
    __loader_##name = {                          \
        .check = checker,                        \
        .load = loader,                          \
}

extern binary_loader_t __binary_loader[];
extern binary_loader_t __binary_loader_end[];

#define foreach_binary_loader()                     \
    for (binary_loader_t *loader = __binary_loader; \
         loader < __binary_loader_end; loader++)
