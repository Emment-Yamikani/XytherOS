#pragma once
#pragma once

#include <core/defs.h>
#include <core/types.h>

extern tid_t gettid(void);
extern pid_t getpid(void);
extern int getcpuid(void);

#define assert(condition, fmt, ...) ({                                                                    \
    if ((condition) == 0)                                                                                 \
        panic("PANIC: %s(): %s:%d: cpu[%d] tid[%d:%d]: ret[%p]: " fmt,                                    \
              __func__, __FILE__, __LINE__, getcpuid(), getpid(), gettid(), __retaddr(0), ##__VA_ARGS__); \
})

#define assert_eq(v0, v1, fmt, ...) ({                                                                    \
    if (((v0) == (v1)) == 0)                                                                              \
        panic("PANIC(#NE): %s(): %s:%d: cpu[%d] tid[%d:%d]: ret[%p]: " fmt,                               \
              __func__, __FILE__, __LINE__, getcpuid(), getpid(), gettid(), __retaddr(0), ##__VA_ARGS__); \
})

#define assert_ne(v0, v1, fmt, ...) ({                                                                    \
    if ((v0) == (v1))                                                                                     \
        panic("PANIC(#EQ): %s(): %s:%d: cpu[%d] tid[%d:%d]: ret[%p]: " fmt,                               \
              __func__, __FILE__, __LINE__, getcpuid(), getpid(), gettid(), __retaddr(0), ##__VA_ARGS__); \
})
