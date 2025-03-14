#include <bits/errno.h>
#include <core/timer.h>
#include <mm/kalloc.h>
#include <sys/thread.h>

static QUEUE(ktimers);

static void ktimer_add(ktimer_t *timer) {
    assert(timer, "Invalid ktimer.\n");

    queue_lock(ktimers);

    embedded_queue_foreach(ktimers, ktimer_t, kt, node) {
        if (timer->expiry <= kt->expiry) {
            embedded_enqueue_before(ktimers, &timer->node, kt_node, QUEUE_UNIQUE);
            queue_unlock(ktimers);
            return;
        }
    }

    embedded_enqueue(ktimers, &timer->node, QUEUE_UNIQUE);

    queue_unlock(ktimers);
}

int ktimer_create(jiffies_t expiry, jiffies_t period, ktimer_callback_t callback, void *arg, timer_t *timerid) {
    ktimer_t *timer;
    static _Atomic(timer_t) id = 1;

    if (!callback && !timerid) {
        return -EINVAL;
    }

    if (NULL == (timer = (ktimer_t *)kmalloc(sizeof *timer))) {
        return -ENOMEM;
    }

    timer->arg      = arg;
    timer->callback = callback;
    timer->period   = period;
    timer->timerid  = atomic_inc(&id);
    timer->expiry   = expiry + jiffies_get();

    *timerid         = timer->timerid;

    ktimer_add(timer);

    return 0;
}

void ktimer_tick(void) {
    queue_lock(ktimers);

    embedded_queue_foreach(ktimers, ktimer_t, kt, node) {
        if (time_after(jiffies_get(), kt->expiry)) {
            kt->callback(kt->arg);

            if (kt->period) {
                kt->expiry += kt->period;
            } else {
                if (!embedded_queue_remove(ktimers, kt_node)) {
                    kfree(kt);
                }
            }
        }
    }

    queue_unlock(ktimers);
}