#pragma once

#include <ds/queue.h>
#include <sync/spinlock.h>

typedef struct rwlock {
    atomic_t    writer;    // Indicator if a writer holds the lock (0 or 1)
    atomic_t    readers;   // Count of readers currently holding the lock
    queue_t     writersq;  // Queue for writers waiting on the lock
    queue_t     readersq;  // Queue for readers waiting on the lock
    spinlock_t  guard;     // Spinlock to protect shared data in the rwlock
} rwlock_t;

#define rwlock_assert(rw)           assert(rw, "Invalid rwlock.")

extern void rwlock_init(rwlock_t *rw);

extern void rw_readlock_acquire(rwlock_t *rw);

extern int rw_readlock_try_acquire(rwlock_t *rw);

extern void rw_readlock_release(rwlock_t *rw);

extern void rw_writelock_acquire(rwlock_t *rw);

extern int rw_writelock_try_acquire(rwlock_t *rw);

extern void rw_writelock_release(rwlock_t *rw);