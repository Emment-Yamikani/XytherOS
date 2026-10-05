#pragma once

#include <core/misc.h>
#include <ds/queue.h>
#include <sync/spinlock.h>
#include <sys/sched/sched_wait.h>

typedef struct cond {
    atomic_t    count;
    queue_t     waiters;
    spinlock_t  lock;
} cond_t;

// #define CONDITION_VARIABLE(ownership, name) ownership cond_t name __used_section(.static_condition_variable) = {0}

#define COND_INLINE_INIT(cond_name) (cond_t){ .count = 0, .lock = SPINLOCK_INIT(), .waiters = QUEUE_INLINE_INIT((cond_name).waiters) }

#define CONDITION_VARIABLE(ownership, cond_name) ownership cond_t cond_name = COND_INLINE_INIT(cond_name)

#define cond_assert(c)           ({ assert(c, "Invalid condition variable.\n"); })
#define cond_lock(c)             ({ cond_assert(c); spin_lock(&(c)->lock); })
#define cond_unlock(c)           ({ cond_assert(c); spin_unlock(&(c)->lock); })
#define cond_assert_locked(c)    ({ cond_assert(c); spin_assert_locked(&(c)->lock); })

extern int     cond_init(cond_t *cond);
extern int     cond_alloc(cond_t **ref);
extern void    cond_free(cond_t *cond);
extern void    cond_signal(cond_t *cond);
extern void    cond_broadcast(cond_t *cond);
extern int     cond_wait(cond_t *cond, wakeup_t *preason, spinlock_t *lock);