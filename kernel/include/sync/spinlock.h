#pragma once

#include <arch/raw_lock.h>
#include <core/defs.h>
#include <lib/printk.h>
#include <sync/preempt.h>
#include <sync/atomic.h>

typedef struct spinlock_t_t {
    arch_raw_lock_t guard;
    uint            locked;
    void            *owner;

    int             line;
    char            *file;
} spinlock_t;

#define SPINLOCK_INIT() ((spinlock_t){ \
    .line       = 0,                   \
    .file       = NULL,                \
    .owner      = NULL,                \
    .locked     = false,               \
})

#define SPINLOCK(name) spinlock_t *name = &SPINLOCK_INIT()

#define spin_assert(lk) assert(lk, "No spinlock.\n")

extern void spinlock_init(spinlock_t *lk);

/* Call with lk->guard held and preemption disabled.*/
extern int holding(const spinlock_t *lk);

#define spin_acquire(lk) ({                                                                  \
    spin_assert(lk);                                                                         \
    pushcli();                                                                               \
                                                                                             \
    arch_raw_lock_acquire(&(lk)->guard);                                                     \
    assert_eq(holding(lk), 0, "Spinlock already held @ [%s:%d].\n", (lk)->file, (lk)->line); \
                                                                                             \
    loop()                                                                                   \
    {                                                                                        \
        if ((lk)->locked == 0)                                                               \
            break;                                                                           \
                                                                                             \
        arch_raw_lock_release(&(lk)->guard);                                                 \
        popcli();                                                                            \
        cpu_pause();                                                                         \
        pushcli();                                                                           \
        arch_raw_lock_acquire(&(lk)->guard);                                                 \
    }                                                                                        \
                                                                                             \
    (lk)->locked = 1;                                                                        \
    (lk)->file = __FILE__;                                                                   \
    (lk)->line = __LINE__;                                                                   \
    (lk)->owner = current ? (void *)current : (void *)cpu;                                   \
                                                                                             \
    arch_raw_lock_release(&(lk)->guard);                                                     \
    memory_barrier();                                                                        \
})

#define spin_lock(lk) ({ \
    spin_assert(lk);     \
    spin_acquire(lk);    \
})

#define spin_release(lk) ({                                \
    spin_assert(lk);                                       \
    pushcli();                                             \
    memory_barrier();                                      \
                                                           \
    arch_raw_lock_acquire(&(lk)->guard);                   \
    assert_eq(holding(lk), 1, "Spinlock must be held.\n"); \
                                                           \
    (lk)->line = 0;                                        \
    (lk)->file = NULL;                                     \
    (lk)->owner = NULL;                                    \
    (lk)->locked = 0;                                      \
    arch_raw_lock_release(&(lk)->guard);                   \
    popcli();                                              \
    popcli();                                              \
})

#define spin_unlock(lk) ({ \
    spin_assert(lk);       \
    spin_release(lk);      \
})

#define spin_islocked(lk) ({             \
    int locked = 0;                      \
                                         \
    spin_assert(lk);                     \
    pushcli();                           \
    arch_raw_lock_acquire(&(lk)->guard); \
    locked = holding(lk);                \
    arch_raw_lock_release(&(lk)->guard); \
    popcli();                            \
    locked;                              \
})

#define spin_recursive_lock(lk) ({     \
    spin_assert(lk);                   \
    int locked;                        \
    if ((locked = !spin_islocked(lk))) \
        spin_lock(lk);                 \
    locked;                            \
})

#define spin_assert_locked(lk) ({                                \
    spin_assert(lk);                                             \
    assert_eq(spin_islocked(lk), 1, "Spinlock must be held.\n"); \
})

#define spin_trylock(lk) ({                                                                  \
    int success = 0;                                                                         \
                                                                                             \
    spin_assert(lk);                                                                         \
    pushcli();                                                                               \
                                                                                             \
    arch_raw_lock_acquire(&(lk)->guard);                                                     \
    assert_eq(holding(lk), 0, "Spinlock already held @ [%s:%d].\n", (lk)->file, (lk)->line); \
                                                                                             \
    if ((lk)->locked == 0)                                                                   \
    {                                                                                        \
        success = 1;                                                                         \
        (lk)->locked = 1;                                                                    \
        (lk)->file = __FILE__;                                                               \
        (lk)->line = __LINE__;                                                               \
        (lk)->owner = current ? (void *)current : (void *)cpu;                               \
    }                                                                                        \
    else                                                                                     \
    {                                                                                        \
        popcli();                                                                            \
    }                                                                                        \
                                                                                             \
    arch_raw_lock_release(&(lk)->guard);                                                     \
    memory_barrier();                                                                        \
    success;                                                                                 \
})
