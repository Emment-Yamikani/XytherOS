#include <lib/printk.h>
#include <bits/errno.h>
#include <sync/cond.h>
#include <mm/kalloc.h>
#include <string.h>
#include <sys/schedule.h>
#include <sys/thread.h>

int cond_init(cond_t *cond) {
    if (cond == NULL)
        return -EINVAL;
    cond->count = 0;
    spinlock_init(&cond->lock);
    return queue_init(&cond->waiters);
}

int cond_alloc(cond_t **ref) {
    int     err = 0;
    cond_t  *cond = NULL;

    if (NULL == (cond = kmalloc(sizeof *cond)))
        return -ENOMEM;

    if ((err = cond_init(cond))) {
        kfree(cond);
        return err;
    }

    *ref = cond;
    return 0;
}

void cond_free(cond_t *c) {
    cond_assert(c);

    cond_lock(c);
    sched_wakeup_all(&c->waiters, NULL);
    cond_unlock(c);

    kfree(c);
}

int cond_wait(cond_t *cond) {
    int err = 0;

    if (cond == NULL)
        return -EINVAL;

    cond_lock(cond);
    if ((int)atomic_inc(&cond->count) >= 0) {
        err = sched_wait(&cond->waiters, T_SLEEP, QUEUE_RELLOC_TAIL, &cond->lock);
    }
    cond_unlock(cond);
    return err;
}

int cond_wait_releasing(cond_t *cond, spinlock_t *lk) {
    int err = 0;
    if (lk) spin_unlock(lk);
    err = cond_wait(cond);
    if (lk) spin_lock(lk);
    return err;
}

static void cond_wake1(cond_t *cond) {
    cond_lock(cond);
    sched_wakeup(&cond->waiters, QUEUE_RELLOC_HEAD);
    atomic_dec(&cond->count);
    cond_unlock(cond);
}

void cond_signal(cond_t *cond) {
    cond_wake1(cond);
}

static void cond_wakeall(cond_t *cond) {
    size_t waiters = 0;
    sched_wakeup_all(&cond->waiters, &waiters);
    if (waiters == 0)
        atomic_write(&cond->count, -1);
    else
        atomic_write(&cond->count, 0);
}

void cond_broadcast(cond_t *cond) {
    cond_lock(cond);
    cond_wakeall(cond);
    cond_unlock(cond);
}