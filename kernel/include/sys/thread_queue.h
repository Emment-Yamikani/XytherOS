#pragma once

#include <ds/queue.h>
#include <sync/spinlock.h>

typedef struct thread_queue_t {
    queue_t         t_queue;
    spinlock_t      t_qlock;
} thread_queue_t;

#define THREAD_QUEUE_NEW()  (thread_queue_t){0}
#define THREAD_QUEUE(name)  thread_queue_t *name = &THREAD_QUEUE_NEW()

#define thread_queue_assert(tq)            ({ assert(tq, "Invalid thread queue."); })
#define thread_queue_lock(tq)              ({ thread_queue_assert(tq); spin_lock(&(tq)->t_qlock); })
#define thread_queue_unlock(tq)            ({ thread_queue_assert(tq); spin_unlock(&(tq)->t_qlock); })
#define thread_queue_islocked(tq)          ({ thread_queue_assert(tq); spin_islocked(&(tq)->t_qlock); })
#define thread_queue_assert_locked(tq)     ({ thread_queue_assert(tq); spin_assert_locked(&(tq)->t_qlock); })


extern int thread_queue_init(thread_queue_t *tqueue);