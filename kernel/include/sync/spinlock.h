#pragma once

#include <sync/atomic.h>
#include <core/defs.h>
#include <sync/preempt.h>
#include <lib/printk.h>

typedef struct spinlock_t_t {
    uint    locked;
    tid_t   owner;
    bool    is_thread;

    int     line;
    char    *file;
} spinlock_t;

#define SPINLOCK_INIT() ((spinlock_t){ \
    .line = 0,                         \
    .file = NULL,                      \
    .locked = false,                   \
    .owner = -1,                       \
    .is_thread = false,                \
})

#define SPINLOCK(name) spinlock_t *name = &SPINLOCK_INIT()

#define spin_assert(lk) assert(lk, "No spinlock.\n")

static inline void spinlock_init(spinlock_t *lk) {
    spin_assert(lk);
    *lk = (spinlock_t){
        .line = 0,
        .file = NULL,
        .locked = false,
        .owner = -1,
        .is_thread = false,
    };
}

// Check whether this cpu/thread is holding the lock.
// Interrupts must be off.
static inline int holding(spinlock_t *lk) {
    int self;
    self = (lk)->is_thread ? (lk)->owner == gettid() : (lk)->owner == getcpuid();
    return (lk)->locked && self;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
#define spin_lock(lk) ({                                                \
    pushcli(); /* disable interrupts to avoid deadlock.*/               \
    assert(!holding(lk), "spinlock already held by this cpu/thread\n"); \
                                                                        \
    while (__sync_lock_test_and_set(&(lk)->locked, 1) != 0)             \
        ;                                                               \
                                                                        \
    __sync_synchronize();                                               \
                                                                        \
    (lk)->file = __FILE__;                                              \
    (lk)->line = __LINE__;                                              \
    (lk)->is_thread = current ? true : false;                           \
    (lk)->owner = current ? gettid() : getcpuid();                      \
})

// Release the lock.
#define spin_unlock(lk) ({                                         \
    pushcli();                                                     \
    assert(holding(lk), "spinlock not held by this cpu/thread\n"); \
                                                                   \
    (lk)->line = 0;                                                \
    (lk)->file = NULL;                                             \
    (lk)->owner = -1;                                              \
    (lk)->is_thread = false;                                       \
                                                                   \
    __sync_synchronize();                                          \
                                                                   \
    __sync_lock_release(&(lk)->locked);                            \
                                                                   \
    popcli();                                                      \
    popcli();                                                      \
})

#define spin_islocked(lk) ({ \
    pushcli();               \
    int state = holding(lk); \
    popcli();                \
    (bool)state;             \
})

#define spin_assert_locked(lk) ({                                   \
    assert_eq(spin_islocked(lk), true, "Caller must hold lock.\n"); \
})

#define spin_test_and_lock(lk) ({      \
    bool locked;                       \
    if ((locked = !spin_islocked(lk))) \
    {                                  \
        spin_lock(lk);                 \
    }                                  \
    locked;                            \
})
