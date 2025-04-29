#pragma once
#pragma once

#include <core/defs.h>
#include <core/types.h>
#include <lib/printk.h>

extern tid_t gettid(void);
extern pid_t getpid(void);
extern int getcpuid(void);
extern thread_t *cpu_get_thread(void);

#define assert(condition, fmt, ...) ({                                                                                   \
    if ((condition) == 0)                                                                                                \
        panic("\e[34mPANIC\e[0m: %s(): %s:%d: cpu[%d] tid[%d:%d]:%p: ret[\e[32m%p\e[0m]: " fmt,                          \
              __func__, __FILE__, __LINE__, getcpuid(), getpid(), gettid(), cpu_get_thread(), __retaddr(0), ##__VA_ARGS__); \
})

#define assert_eq(v0, v1, fmt, ...) ({                                                                                                      \
    typeof(v0) lhs = v0;                                                                                                                    \
    typeof(lhs) rhs = (typeof(lhs))v1;                                                                                                      \
    if (((lhs) == (rhs)) == 0)                                                                                                              \
        panic("\e[34mPANIC(#NE: lhs=\e[33m%p\e[0m : rhs=\e[33m%p\e[0m)\e[0m: %s(): %s:%d: cpu[%d] tid[%d:%d]:%p: ret[\e[32m%p\e[0m]: " fmt, \
              (lhs), (rhs), __func__, __FILE__, __LINE__, getcpuid(), getpid(), gettid(), cpu_get_thread(), __retaddr(0), ##__VA_ARGS__);      \
})

#define assert_ne(v0, v1, fmt, ...) ({                                                                                                      \
    typeof(v0) lhs = v0;                                                                                                                    \
    typeof(lhs) rhs = (typeof(lhs))v1;                                                                                                      \
    if ((lhs) == (rhs))                                                                                                                     \
        panic("\e[34mPANIC(#EQ: lhs=\e[33m%p\e[0m : rhs=\e[33m%p\e[0m)\e[0m: %s(): %s:%d: cpu[%d] tid[%d:%d]:%p: ret[\e[32m%p\e[0m]: " fmt, \
              (lhs), (rhs), __func__, __FILE__, __LINE__, getcpuid(), getpid(), gettid(), cpu_get_thread(), __retaddr(0), ##__VA_ARGS__);      \
})
