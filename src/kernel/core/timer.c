#include <bits/errno.h>
#include <core/debug.h>
#include <mm/kalloc.h>
#include <sys/thread.h>

static QUEUE(ktimer_queue);

static int compare_timer_expiry(queue_node_t *x, queue_node_t *y) {
    posix_timer_t *tx, *ty;

    if (!x || !y) {
        return -EINVAL;
    }

    queue_assert_locked(ktimer_queue);

    tx = queue_node_get_container(x, posix_timer_t, knode);
    ty = queue_node_get_container(y, posix_timer_t, knode);

    if (tx->expiry_time == ty->expiry_time)
        return QUEUE_EQUAL;
    else if (tx->expiry_time < ty->expiry_time)
        return QUEUE_LESSER;
    return QUEUE_GREATER;
}

static void add_timer_to_kernel_queue(posix_timer_t *timer) {
    // Insert the timer into the queue in order of expiry time
    queue_lock(ktimer_queue);
    embedded_enqueue_sorted(
        ktimer_queue,
        &timer->knode,
        QUEUE_UNIQUE, QUEUE_ASCENDING,
        compare_timer_expiry
    );
    queue_unlock(ktimer_queue);
}

static void remove_timer_from_kernel_queue(posix_timer_t *timer) {
    queue_lock(ktimer_queue);
    embedded_queue_detach(ktimer_queue, &timer->knode);
    queue_unlock(ktimer_queue);
}

static int add_timer_to_process(thread_t *thread, posix_timer_t *timer) {
    if (!thread || !timer) {
        return -EINVAL;
    }

    queue_lock(thread->t_timers);
    int err = embedded_enqueue(thread->t_timers, &timer->node, QUEUE_UNIQUE);
    queue_unlock(thread->t_timers);
    return err;
}

static void remove_timer_from_process(thread_t *thread, posix_timer_t *timer) {
    queue_lock(thread->t_timers);
    embedded_queue_detach(thread->t_timers, &timer->node);
    queue_unlock(thread->t_timers);
}

static posix_timer_t *find_timer_by_id(timer_t timerid) {
    posix_timer_t *timer;

    queue_lock(current->t_timers);
    queue_foreach_entry(current->t_timers, timer, node) {
        spin_lock(&timer->lock);
        if (timer->id == timerid) {
            queue_unlock(current->t_timers);
            return timer;
        }
        spin_unlock(&timer->lock);
    }
    queue_unlock(current->t_timers);
    return NULL;
}

static posix_timer_t *get_expired_timer(void) {
    posix_timer_t *timer;

    queue_lock(ktimer_queue);
    queue_foreach_entry(ktimer_queue, timer, knode) {
        spin_lock(&timer->lock);
        if (timer->expiry_time <= jiffies_get()) {
            embedded_queue_detach(ktimer_queue, timer_node);
            queue_unlock(ktimer_queue);
            return timer;
        }
        spin_unlock(&timer->lock);
    }
    queue_unlock(ktimer_queue);
    return NULL;
}

static void deliver_signal_or_event(thread_t *thread, sigevent_t *event) {
    if (event->sigev_notify == SIGEV_SIGNAL) {
        thread_lock(thread);
        thread_send_signal(thread, event->sigev_signo, event->sigev_value);
        thread_unlock(thread);
    } else if (event->sigev_notify == SIGEV_THREAD) {
        thread_create(
            event->sigev_attribute,
            (void *)event->sigev_function,
            event->sigev_value.sigval_ptr,
            THREAD_CREATE_SCHED, NULL
        );
    } else if (event->sigev_notify == SIGEV_THREAD_ID) {
        pthread_kill(event->sigev_tid, event->sigev_signo);
    } else if (event->sigev_notify == SIGEV_CALLBACK) {
        event->sigev_function(event->sigev_attribute);
    }
}

static void timer_worker(void) {
    loop_and_yield() {
        posix_timer_t *timer = get_expired_timer();
        if (!timer) {
            sched_yield();
            continue;
        }

        // Deliver the signal or event.
        deliver_signal_or_event(timer->owner, &timer->event);

        // If the timer is periodic, re-arm it.
        if (timer->interval > 0) {
            timer->expiry_time += timer->interval;
            add_timer_to_kernel_queue(timer);
        }
        spin_unlock(&timer->lock);
    }
} BUILTIN_THREAD(timer_worker, timer_worker, NULL);

static timer_t generate_unique_timer_id(void) {
    static _Atomic(timer_t) timer_id = 0;
    return atomic_inc(&timer_id);
}

int timer_create_r(thread_t *owner, clockid_t clockid, sigevent_t *sevp, timer_t *timerid) {
    int ret = posix_timer_validate_clockid(clockid);
    if (ret < 0) {
        return ret;
    }

    if (!owner) {
        return -EINVAL;
    }

    // Allocate a new timer structure
    posix_timer_t *timer = kmalloc(sizeof(posix_timer_t));
    if (!timer) {
        return -ENOMEM;
    }

    // Initialize the timer fields
    timer->interval     = 0;
    timer->expiry_time  = 0;
    timer->owner        = owner;
    timer->clockid      = clockid;
    timer->event = sevp ? *sevp : (sigevent_t) {
        .sigev_signo    = SIGALRM,
        .sigev_notify   = SIGEV_SIGNAL,
        .sigev_value    = (sigval_t){0},
        .sigev_function = NULL,
        .sigev_attribute= NULL
    };

    if (timer->event.sigev_notify == SIGEV_CALLBACK || timer->event.sigev_notify == SIGEV_THREAD) {
        if (timer->event.sigev_function == NULL) {
            kfree(timer);
            return -EINVAL;
        }
    }

    timer->id = generate_unique_timer_id();
    // Add the timer to the process's timer list
    add_timer_to_process(owner, timer);

    *timerid = timer->id;
    return 0;
}

int timer_create(clockid_t clockid, sigevent_t *sevp, timer_t *timerid) {
    return timer_create_r(current, clockid, sevp, timerid);
}

int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value, struct itimerspec *old_value) {
    if (!new_value) {
        return -EFAULT;
    }

    if (new_value->it_value.tv_sec < 0) {
        return -EINVAL;
    }

    if (new_value->it_value.tv_nsec < 0 || new_value->it_value.tv_nsec > 999999999l) {
        return -EINVAL;
    }

    posix_timer_t *timer = find_timer_by_id(timerid);
    if (!timer) {
        return -EINVAL;
    }

    // Save the old timer settings if requested
    if (old_value) {
        jiffies_to_timespec(timer->interval, &old_value->it_interval);
        jiffies_to_timespec(timer->expiry_time, &old_value->it_value);
    }

    // Set the new timer settings
    timer->expiry_time = jiffies_from_timespec(&new_value->it_value);
    timer->expiry_time += flags & POSIX_TIMER_ABSTIME ? 0 : jiffies_get();
    timer->interval = jiffies_from_timespec(&new_value->it_interval);

    // Add the timer to the kernel's timer queue
    add_timer_to_kernel_queue(timer);
    spin_unlock(&timer->lock);

    return 0;
}

int timer_gettime(timer_t timerid, struct itimerspec *curr_value) {
    if (!curr_value) {
        return -EFAULT;
    }

    posix_timer_t *timer = find_timer_by_id(timerid);
    if (!timer) {
        return -EINVAL;
    }

    jiffies_to_timespec(timer->interval, &curr_value->it_interval);
    jiffies_to_timespec(timer->expiry_time, &curr_value->it_value);

    spin_unlock(&timer->lock);
    return 0;
}

int timer_delete(timer_t timerid) {
    posix_timer_t *timer = find_timer_by_id(timerid);
    if (!timer) {
        return -EINVAL;
    }

    // Remove the timer from the kernel's timer queue
    remove_timer_from_kernel_queue(timer);

    // Remove the timer from the process's timer list
    remove_timer_from_process(timer->owner, timer);
    spin_unlock(&timer->lock);

    // Free the timer structure
    kfree(timer);

    return 0;
}