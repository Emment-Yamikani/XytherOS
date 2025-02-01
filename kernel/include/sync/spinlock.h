#pragma once

#include <sync/atomic.h>
#include <core/defs.h>
#include <sync/preempt.h>
#include <lib/printk.h>

typedef struct spinlock_t {
    bool    guard;
    bool    locked;
    tid_t   owner_id;
    bool    is_thread;

    int     line;
    char    *file;
} spinlock_t;

#define SPINLOCK_INIT() ((spinlock_t){ \
    .line = 0,                         \
    .file = NULL,                      \
    .locked = false,                   \
    .owner_id = -1,                    \
    .is_thread = false,                \
})

#define SPINLOCK(name) spinlock_t *name = &SPINLOCK_INIT()

#define spin_assert(lk) assert(lk, "No spinlock.\n")

#define spin_lock(lk) ({                                                                         \
    compiler_barrier(); /* Prevent compiler reordering */                                        \
    spin_assert(lk);                                                                             \
    pushcli(); /* Disable interrupts */                                                          \
    for (;;)                                                                                     \
    { /* Infinite loop until lock is acquired */                                                 \
        bool expected = false;                                                                   \
        /* Acquire the guard flag atomically */                                                  \
        while (!atomic_cmpxchg(&(lk)->guard, &expected, true))                                   \
        {                                                                                        \
            popcli();    /* Re-enable interrupts while waiting */                                \
            cpu_pause(); /* Pause to reduce CPU contention */                                    \
            pushcli();   /* Disable interrupts again */                                          \
        }                                                                                        \
        /* Check if the lock is available */                                                     \
        if (!(lk)->locked)                                                                       \
        {                                                                                        \
            break; /* Lock is free, proceed to acquire it */                                     \
        }                                                                                        \
        /* Check if the lock is already held by the current thread/CPU */                        \
        bool self = (lk)->is_thread ? (lk)->owner_id == gettid() : (lk)->owner_id == getcpuid(); \
        assert(!self, "Spinlock already held by this thread/CPU.\n");                            \
        /* Release the guard flag and retry */                                                   \
        atomic_clear(&(lk)->guard);                                                              \
    }                                                                                            \
    /* Update lock metadata */                                                                   \
    (lk)->line = __LINE__;                                                                       \
    (lk)->file = __FILE__;                                                                       \
    (lk)->is_thread = (current != NULL);                                                         \
    (lk)->owner_id = (current != NULL) ? gettid() : getcpuid();                                  \
    (lk)->locked = true;        /* Mark the lock as acquired */                                  \
    atomic_clear(&(lk)->guard); /* Release the guard flag (includes memory barrier) */           \
    compiler_barrier();                                                                          \
})

#define spin_assert_locked(lk) ({                                                                \
    compiler_barrier(); /* Prevent compiler reordering */                                        \
    pushcli();                                                                                   \
    spin_assert(lk);                                                                             \
    bool expected = false;                                                                       \
    /* Temporarily acquire the guard flag to check state */                                      \
    while (!atomic_cmpxchg(&(lk)->guard, &expected, true))                                       \
    {                                                                                            \
        popcli();                                                                                \
        cpu_pause();                                                                             \
        pushcli();                                                                               \
    }                                                                                            \
    /* Check if the lock is held */                                                              \
    bool is_locked = (lk)->locked;                                                               \
    /* Check if the current thread/CPU is the owner */                                           \
    bool is_owner = (lk)->is_thread ? (lk)->owner_id == gettid() : (lk)->owner_id == getcpuid(); \
    /* Release the guard flag */                                                                 \
    atomic_clear(&(lk)->guard);                                                                  \
    /* Assert ownership */                                                                       \
    assert(is_locked &&is_owner, "Spinlock not held by current thread/CPU.\n");                  \
    popcli();                                                                                    \
    compiler_barrier();                                                                          \
})

#define spin_islocked(lk) ({                                                                   \
    compiler_barrier(); /* Prevent compiler reordering */                                      \
    pushcli();                                                                                 \
    spin_assert(lk);                                                                           \
    bool is_locked = false;                                                                    \
    bool by_self = false;                                                                      \
    bool expected = false;                                                                     \
    /* Acquire the guard flag to read the lock state */                                        \
    while (!atomic_cmpxchg(&(lk)->guard, &expected, true))                                     \
    {                                                                                          \
        popcli();                                                                              \
        cpu_pause();                                                                           \
        pushcli();                                                                             \
    }                                                                                          \
    is_locked = (lk)->locked;                                                                  \
    is_locked = (lk)->locked;                                                                  \
    if (is_locked)                                                                             \
        by_self = (lk)->is_thread ? (lk)->owner_id == gettid() : (lk)->owner_id == getcpuid(); \
    /* Release the guard flag */                                                               \
    atomic_clear(&(lk)->guard);                                                                \
    popcli();                                                                                  \
    compiler_barrier();                                                                        \
    is_locked &&by_self; /* Return the lock state */                                           \
})

#define spin_unlock(lk) ({                                 \
    compiler_barrier(); /* Prevent compiler reordering */  \
    pushcli();                                             \
    spin_assert_locked(lk);                                \
    bool expected = false;                                 \
    /* Acquire the guard flag atomically */                \
    while (!atomic_cmpxchg(&(lk)->guard, &expected, true)) \
    {                                                      \
        popcli();                                          \
        cpu_pause();                                       \
        pushcli();                                         \
    }                                                      \
    /* Mark the lock as released */                        \
    (lk)->locked = false;                                  \
    /* Clear owner metadata */                             \
    (lk)->is_thread = false;                               \
    (lk)->owner_id = -1;                                   \
    /* Release the guard flag (includes memory barrier) */ \
    atomic_clear(&(lk)->guard);                            \
    popcli(); /* Reverse pushcli() done in spin_lock() */  \
    popcli(); /* Re-enable interrupts */                   \
    compiler_barrier();                                    \
})
