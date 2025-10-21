#include <bits/errno.h>
#include <dev/timer.h>
#include <mm/kalloc.h>
#include <sys/schedule.h>
#include <sys/thread.h>

static QUEUE(jiffies_clocks);
static QUEUE(jiffies_waiters);
static CONDITION_VARIABLE(jiffies_condvar);
static volatile _Atomic(jiffies_t) jiffies_now = 0;

typedef struct {
    tid_t     tid;
    jiffies_t jiffies;
    queue_node_t node;
} jiffies_clock_t;

static int compare(queue_node_t *a, queue_node_t *b) {
    if (!a || !b) {
        return -EINVAL;
    }

    jiffies_clock_t *aclock, *bclock;
    aclock = queue_node_get_container(a, jiffies_clock_t, node);
    bclock = queue_node_get_container(b, jiffies_clock_t, node);

    if (aclock->jiffies == bclock->jiffies) {
        return QUEUE_EQUAL;
    } else if (aclock->jiffies < bclock->jiffies) {
        return QUEUE_LESSER;
    }

    return QUEUE_GREATER;
}

int jiffies_create_clock(jiffies_t jiffy) {
    jiffies_clock_t *clock = kmalloc(sizeof *clock);

    if (clock == NULL) {
        return -ENOMEM;
    }

    clock->jiffies  = jiffy;
    clock->tid      = gettid();

    queue_lock(jiffies_clocks);
    int err = enqueue_sorted(
        jiffies_clocks,
        &clock->node,
        QUEUE_UNIQUE,
        QUEUE_ASCENDING,
        compare
    );
    queue_unlock(jiffies_clocks);

    if (err) {
        kfree(clock);
        return err;
    }

    return 0;
}

static void jiffies_worker(void) {
    loop_and_yield() {
        cond_wait(jiffies_condvar, NULL, NULL);
        queue_lock(jiffies_clocks);
        jiffies_clock_t *clock;
        queue_foreach_entry(jiffies_clocks, clock, node) {
            if (time_after(jiffies_get(), clock->jiffies)) {
                embedded_queue_detach(jiffies_clocks, clock_node);
                sched_wakeup_specific(jiffies_waiters, WAKEUP_NORMAL, clock->tid);
                kfree(clock);
            }
        }
        queue_unlock(jiffies_clocks);
    }
} BUILTIN_THREAD(jiffies_worker, jiffies_worker, NULL);

void jiffies_update(void) {
    if ((atomic_inc_fetch(&jiffies_now) % SYS_Hz) == 0) {
        epoch_update();
    }

    cond_signal(jiffies_condvar);
}

jiffies_t jiffies_get(void) {
    return atomic_read(&jiffies_now);
}

void jiffies_timed_wait(jiffies_t jiffies) {
    jiffies_t jiffy = jiffies_get() + jiffies;
    while (time_before(jiffies_get(), jiffy));
}

void jiffies_to_timespec(jiffies_t jiffies, struct timespec *ts) {
    ts->tv_sec = jiffies / SYS_Hz;
    ts->tv_nsec= (jiffies % SYS_Hz) * 1000000;
}

jiffies_t jiffies_from_timespec(const struct timespec *ts) {
    return (ts->tv_sec * SYS_Hz) + (ts->tv_nsec / 1000000);
}

int jiffies_getres(struct timespec *res) {
    if (res == NULL) {
        return -EINVAL;
    }

    res->tv_sec = 0;
    res->tv_nsec= 1e9l / SYS_Hz;

    return 0;
}

int jiffies_gettime(struct timespec *tp) {
    if (tp == NULL) {
        return -EINVAL;
    }

    jiffies_to_timespec(jiffies_get(), tp);
    return 0;
}

int jiffies_sleep(jiffies_t jiffies, jiffies_t *rem) {
    int err = 0;

    jiffies_t expr = jiffies + jiffies_get(), now;
    while (time_before(now = jiffies_get(), expr)) {
        if (!jiffies_create_clock(expr)) {
            if ((err = sched_wait_whence(jiffies_waiters, T_SLEEP, QUEUE_HEAD, NULL, NULL))) {
                now = jiffies_get();
                break;
            }
        }
    }

    if (rem) {
        *rem = (long)(expr - now) < 0 ? 0 : expr - now;
    }

    return err;
}