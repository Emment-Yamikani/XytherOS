#pragma once

#include <core/assert.h>

extern tid_t gettid(void);
extern pid_t getpid(void);

#define log_error(fmt, ...)                                                 \
    printk("\e[34mERROR\e[0m: %s:%d: cpu[%d:%d]: tid[%d:%d]: ret[\e[32m%p\e[0m] " fmt, \
           __func__, __LINE__, getcpuid(), cpu->ncli, getpid(), gettid(), __retaddr(0), ##__VA_ARGS__)

#ifdef DEBUG_BUILD
#define debug(fmt, ...) printk("\e[33mDEBUG\e[0m: %s:%d: cpu[%d:%d]: tid[%d:%d]: ret[\e[32m%p\e[0m] " fmt, \
                               __func__, __LINE__, getcpuid(), cpu->ncli, getpid(), gettid(), __retaddr(0), ##__VA_ARGS__)
#else
#define debug(fmt, ...) // No-op in release builds
#endif

#ifdef DEBUG_BUILD
#define todo(fmt, ...) panic("\e[34mTODO\e[0m: %s:%d: cpu[%d:%d]: tid[%d:%d]: ret[\e[32m%p\e[0m] " fmt, \
                             __func__, __LINE__, getcpuid(), cpu->ncli, getpid(), gettid(), __retaddr(0), ##__VA_ARGS__)
#else
#define todo(fmt, ...) // No-op in release builds
#endif

#define debuglog() ({ \
    debug("\n");      \
})

#ifndef dumpf
#define dumpf(hlt, fmt, ...)                                                                               \
    if (hlt)                                                                                               \
        panic("\e[34mDUMP\e[0m: %s:%d: cpu[%d:%d]: tid[%d:%d]: ret[\e[32m%p\e[0m]\n" fmt,                  \
              __func__, __LINE__, getcpuid(), cpu->ncli, getpid(), gettid(), __retaddr(0), ##__VA_ARGS__); \
    else                                                                                                   \
        printk("DUMP: %s:%d: cpu[%d:%d]: tid[%d:%d]: ret[\e[32m%p\e[0m]\n" fmt,                            \
               __func__, __LINE__, getcpuid(), cpu->ncli, getpid(), gettid(), __retaddr(0), ##__VA_ARGS__)
#endif
