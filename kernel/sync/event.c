#include <bits/errno.h>
#include <mm/kalloc.h>
#include <sync/event.h>
#include <sys/schedule.h>
#include <core/debug.h>
#include <sys/thread.h>

int await_event_init(await_event_t *ev) {
    if (ev == NULL) {
        return -EINVAL;
    }

    ev->ev_count = 0;
    ev->ev_triggered = false;
    ev->ev_broadcast = false;
    spinlock_init(&ev->ev_lock);
    queue_init(&ev->ev_waitqueue);
    return 0;
}

void await_event_destroy(await_event_t *ev) {
    if (ev == NULL) {
        return;
    }

    spin_lock(&ev->ev_lock);
    /* Wake all waiters with error */
    ev->ev_count = 0;
    ev->ev_broadcast = false;
    ev->ev_triggered = false;

    sched_wakeup_all(&ev->ev_waitqueue, WAKEUP_ERROR, NULL);
    spin_unlock(&ev->ev_lock);
}

int await_event(await_event_t *ev, spinlock_t *spinlock) {
    return await_event_timed(ev, NULL, spinlock);
}

int await_event_timed(await_event_t *ev, const timespec_t *timeout, spinlock_t *spinlock) {
    if (ev == NULL) {
        return -EINVAL;
    }

    int err = 0;
    spin_lock(&ev->ev_lock);

    if (ev->ev_triggered) {
        if (ev->ev_count > 0) {
            if (--ev->ev_count == 0) {
                ev->ev_triggered = false;
            }
        }

        spin_unlock(&ev->ev_lock);
        return 0;
    }

    if (timeout == NULL) {
        while (!ev->ev_triggered) {
            spin_unlock(&ev->ev_lock);
            err = sched_wait(&ev->ev_waitqueue, T_SLEEP, spinlock);
            spin_lock(&ev->ev_lock);
            if (err) goto error;
        }
    } else {
        jiffies_t deadline = jiffies_get() + jiffies_from_timespec(timeout);
        while (!ev->ev_triggered && !time_after_eq(jiffies_get(), deadline)) {
            spin_unlock(&ev->ev_lock);
            err = sched_wait(&ev->ev_waitqueue, T_SLEEP, spinlock);
            spin_lock(&ev->ev_lock);
            if (err) goto error;
        }

        if (!ev->ev_triggered) {
            spin_unlock(&ev->ev_lock);
            return -ETIMEDOUT;
        }
    }

    if (ev->ev_broadcast) {
        // With broadcast, we don't decrement count until all waiters are processed
        if (sched_wait_queue_length(&ev->ev_waitqueue) == 0) {
            if (--ev->ev_count == 0) {
                ev->ev_triggered = false;
            }
            ev->ev_broadcast = false;
        }
    } else {
        if (--ev->ev_count == 0) {
            ev->ev_triggered = false;
        }
    }

    spin_unlock(&ev->ev_lock);
    return 0;
error:
    spin_unlock(&ev->ev_lock);
    return err;
}

int try_await_event(await_event_t *ev) {
    if (ev == NULL) {
        return -EINVAL;
    }

    int ret = 0;

    spin_lock(&ev->ev_lock);

    if (ev->ev_triggered) {
        if (ev->ev_count > 0) {
            if (--ev->ev_count == 0) {
                ev->ev_triggered = false;
            }
        }
        ret = 1;
    }

    spin_unlock(&ev->ev_lock);
    return ret;
}

void await_event_signal(await_event_t *ev) {
    if (ev == NULL) {
        return;
    }

    spin_lock(&ev->ev_lock);

    ev->ev_count++;
    ev->ev_triggered = true;
    ev->ev_broadcast = false;

    sched_wakeup(&ev->ev_waitqueue, WAKEUP_NORMAL);
    spin_unlock(&ev->ev_lock);
}

void await_event_broadcast(await_event_t *ev) {
    if (ev == NULL) {
        return;
    }

    size_t waiters = 0;
    spin_lock(&ev->ev_lock);

    ev->ev_count++;
    ev->ev_triggered = true;
    
    sched_wakeup_all(&ev->ev_waitqueue, WAKEUP_NORMAL, &waiters);
    
    if (waiters > 0) {
        ev->ev_broadcast = true;
    }

    spin_unlock(&ev->ev_lock);
}
