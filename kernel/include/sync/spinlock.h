#pragma once

#include <arch/raw_lock.h>
#include <core/defs.h>
#include <lib/printk.h>
#include <sync/preempt.h>
#include <sync/atomic.h>

typedef struct spinlock_t_t {
    arch_raw_lock_t guard;
    uint            locked;
    int             threaded;
    void            *owner;

    int     line;
    char    *file;
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

extern void spin_acquire(spinlock_t *lk);
extern void spin_lock(spinlock_t *lk);
extern int spin_trylock(spinlock_t *lk);
extern int spin_test_and_lock(spinlock_t *lk);

extern void spin_release(spinlock_t *lk);
extern void spin_unlock(spinlock_t *lk);

extern int spin_islocked(spinlock_t *lk);

extern void spin_assert_locked(spinlock_t *lk);