#include <bits/errno.h>
#include <core/debug.h>
#include <core/timer.h>
#include <mm/kalloc.h>
#include <string.h>
#include <sys/schedule.h>

static QUEUE(timers);
static CONDITION_VARIABLE(timer_waiters);

static int timer_new(tmr_t **pt) {
    tmr_t *tmr;

    if (pt == NULL) {
        return -EINVAL;
    }

    if (NULL == (tmr = kmalloc(sizeof *tmr))) {
        return -ENOMEM;
    }

    memset(tmr, 0, sizeof *tmr);

    *pt = tmr;

    return 0;
}

static void init_timer(tmr_t *tmr, tmr_desc_t *td) {
    tmr->t_owner    = current;
    tmr->t_arg      = td->td_arg;
    tmr->t_callback = td->td_func;
    tmr->t_signo    = td->td_signo;
    tmr->t_interval = jiffies_from_timespec(&td->td_inteval);
    tmr->t_expiry   = jiffies_get() + jiffies_from_timespec(&td->td_expiry);
}

static void timer_free(tmr_t *tmr) {
    if (tmr) {
        kfree(tmr);
    }
}

static int timer_register(tmr_t *tmr) {
    int err;
    static _Atomic(tmrid_t) tmrid = 0;

    if (tmr == NULL) {
        return -EINVAL;
    }

    // TODO: check availability of tmr->owner.

    queue_lock(timers);
    err = embedded_enqueue(timers, &tmr->t_node, QUEUE_UNIQUE);
    queue_unlock(timers);

    if (err == 0) {
        tmr->t_id = atomic_inc(&tmrid);
    }

    return err;
}

int timer_create(tmr_desc_t *td, int *ptid) {
    int err;
    tmr_t *tmr = NULL;

    if (!td || (!td->td_func && !td->td_signo)) {
        return -EINVAL;
    }

    if ((err = timer_new(&tmr))) {
        return err;
    }

    init_timer(tmr, td);

    if ((err = timer_register(tmr))) {
        goto error;
    }

    *ptid = tmr->t_id;

    return 0;

error:
    timer_free(tmr);
    return err;
}

void timer_increment(void) {
    cond_broadcast(timer_waiters);

    queue_lock(timers);
    embedded_queue_foreach(timers, tmr_t, tmr, t_node) {
        if (time_after(jiffies_get(), tmr->t_expiry)) {
            if (tmr->t_callback) {
                tmr->t_callback(tmr->t_arg);
            } else if (tmr->t_owner) {
                thread_lock(tmr->t_owner);
                thread_kill(tmr->t_owner, tmr->t_signo, (union sigval){0});
                thread_unlock(tmr->t_owner);
            }

            if (tmr->t_interval) {
                tmr->t_expiry += tmr->t_interval;
            } else {
                if (!embedded_queue_remove(timers, &tmr->t_node)) {
                    timer_free(tmr);
                }
            }
        }
    }
    queue_unlock(timers);
}

int nanosleep(const timespec_t *duration, timespec_t *rem) {
    int         err = 0;
    jiffies_t   time, now;

    if (duration == NULL) {
        return -EFAULT;
    }

    if ((duration->tv_nsec > 999999999) || (duration->tv_sec < 0)) {
        return -EINVAL;
    }

    time = jiffies_from_timespec(duration) + jiffies_get();

    while (time_before(now = jiffies_get(), time)) {
        if ((err = cond_wait(timer_waiters))) {
            break;
        }
    }

    if (rem) {
        time = now <= time ? time - now : 0;
        jiffies_to_timespec(time, rem);
    }

    return err;
}