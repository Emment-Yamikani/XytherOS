#include <bits/errno.h>
#include <mm/kalloc.h>
#include <sync/event.h>
#include <sys/schedule.h>
#include <sys/thread.h>

int await_event_init(await_event_t *ev) {
    if (ev == NULL)
        return -EINVAL;

    ev->count = 0;
    spinlock_init(&ev->lock);
    queue_init(&ev->waitqueue);
    return 0;
}

int try_await_event(await_event_t *ev) {
    if (ev == NULL)
        return -EINVAL;

    spin_lock(&ev->lock);
    if (ev->count <= 0) {
        spin_unlock(&ev->lock);
        return -EAGAIN;
    }
    ev->count--;
    spin_unlock(&ev->lock);
    return 0;
}

int await_event(await_event_t *ev) {
    return await_event_timed(ev, NULL);
}

int await_event_timed(await_event_t *ev, const timespec_t *timeout) {
    if (ev == NULL)
        return -EINVAL;

    spin_lock(&ev->lock);
    
    /* Fast path: event immediately available */
    if (ev->count > 0) {
        ev->count--;
        spin_unlock(&ev->lock);
        return 0;
    }

    /* Need to wait - register as waiter */
    long old_count = ev->count;
    ev->count--;
    int ret = 0;

    /* Wait loop */
    if (timeout == NULL) {
        while (ev->count == old_count) {  // No wakeup occurred
            ret = sched_wait(&ev->waitqueue, T_SLEEP, &ev->lock);
            if (ret != 0)
                break;
        }
    } else {
        time_t deadline = jiffies_from_timespec(timeout) + jiffies_get();
        while (ev->count == old_count && time_after(deadline, jiffies_get())) {
            ret = sched_wait(&ev->waitqueue, T_SLEEP, &ev->lock);
            if (ret != 0)
                break;
        }
        if (ev->count == old_count && !time_after(deadline, jiffies_get()))
            ret = -ETIMEDOUT;
    }

    /* Check if we got an event */
    if (ret == 0 && ev->count > old_count) {
        spin_unlock(&ev->lock);
        return 0;
    }

    /* Cleanup if wait failed */
    if (ret != 0) {
        ev->count++;  // Remove our waiter registration
    }
    spin_unlock(&ev->lock);
    return ret;
}

void await_event_wakeup(await_event_t *ev) {
    if (ev == NULL)
        return;

    spin_lock(&ev->lock);
    if (ev->count < 0) {
        /* Wake one waiter */
        sched_wakeup(&ev->waitqueue, WAKEUP_NORMAL, QUEUE_HEAD);
        ev->count++;  // One less waiter (count moves toward zero)
    } else {
        /* No waiters - just record the event */
        ev->count++;
    }
    spin_unlock(&ev->lock);
}

void await_event_wakeup_all(await_event_t *ev) {
    if (ev == NULL)
        return;

    spin_lock(&ev->lock);
    if (ev->count < 0) {
        /* Wake all waiters and give each one event */
        long waiters = -ev->count;
        ev->count = waiters;  // Each waiter gets one event
        sched_wakeup_all(&ev->waitqueue, WAKEUP_NORMAL, NULL);
    } else {
        /* No waiters - just record the event */
        ev->count++;
    }
    spin_unlock(&ev->lock);
}

void await_event_destroy(await_event_t *ev) {
    if (ev == NULL)
        return;

    spin_lock(&ev->lock);
    /* Wake all waiters with error */
    if (ev->count < 0) {
        sched_wakeup_all(&ev->waitqueue, WAKEUP_ERROR, NULL);
        ev->count = 0;
    }
    spin_unlock(&ev->lock);
}
