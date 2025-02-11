#pragma once

#include <sys/thread_queue.h>
#include <sync/spinlock.h>

struct thread_queue_t;
typedef struct cond {
    atomic_t        count;
    struct thread_queue_t  waiters;

    spinlock_t      lock;
} cond_t;

#define COND_INIT()                 ((cond_t){0})
#define COND_NEW()                  (&COND_INIT())
#define COND_VAR(name)              cond_t *name = COND_NEW()
#define CONDITION_VARIABLE(name)    cond_t *name = COND_NEW()

#define cond_assert(c)              ({ assert(c, "Invalid condition variable.\n"); })
#define cond_lock(c)                ({ cond_assert(c); spin_lock(&(c)->lock); })
#define cond_unlock(c)              ({ cond_assert(c); spin_unlock(&(c)->lock); })
#define cond_assert_locked(c)       ({ cond_assert(c); spin_assert_locked(&(c)->lock); })

extern int     cond_init(cond_t *cond);
extern int     cond_new(cond_t **ref);
extern int     cond_wait(cond_t *cond);
extern void    cond_free(cond_t *cond);
extern void    cond_signal(cond_t *cond);
extern void    cond_broadcast(cond_t *cond);
extern int     cond_wait_releasing(cond_t *cond, spinlock_t *lk);