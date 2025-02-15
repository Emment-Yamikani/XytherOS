#pragma once

#include <core/assert.h>

extern tid_t gettid(void);
extern pid_t getpid(void);

#define log_error(fmt, ...)                                      \
    printk("ERROR: %s:%d: cpu[%d:%d]: tid[%d:%d]: ret[%p] " fmt, \
           __func__, __LINE__, getcpuid(), cpu->ncli, getpid(), gettid(), __retaddr(0), ##__VA_ARGS__)

#ifdef DEBUG_BUILD
#define debug(fmt, ...) printk("DEBUG: %s:%d: cpu[%d:%d]: tid[%d:%d]: ret[%p] " fmt, \
                               __func__, __LINE__, getcpuid(), cpu->ncli, getpid(), gettid(), __retaddr(0), ##__VA_ARGS__)
#else
#define debug(fmt, ...) // No-op in release builds
#endif

#ifdef DEBUG_BUILD
#define todo(fmt, ...) panic("TODO: %s:%d: cpu[%d:%d]: tid[%d:%d]: ret[%p] " fmt, \
                             __func__, __LINE__, getcpuid(), cpu->ncli, getpid(), gettid(), __retaddr(0), ##__VA_ARGS__)
#else
#define todo(fmt, ...) // No-op in release builds
#endif

#define debuglog() ({ \
    debug("\n");      \
})
