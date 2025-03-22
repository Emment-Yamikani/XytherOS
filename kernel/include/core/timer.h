#pragma once

#include <core/types.h>
#include <dev/timer.h>
#include <sys/_signal.h>
#include <sys/_time.h>
#include <sync/spinlock.h>

#define NTIMERS_PER_PROC    8

enum {
    CLOCK_REALTIME = 1,
    CLOCK_MONOTONIC,
    CLOCK_PROCESS_CPUTIME_ID,
    CLOCK_THREAD_CPUTIME_ID
};

struct itimerspec {
    struct timespec it_interval;  // Interval for periodic timer
    struct timespec it_value;     // Initial expiration time
};

typedef struct posix_timer {
    timer_t         id;         // Unique timer ID
    clockid_t       clockid;    // Timer type (e.g., CLOCK_REALTIME)
    jiffies_t       expiry_time;// Absolute expiration time (in jiffies)
    jiffies_t       interval;   // Periodic interval (0 for one-shot)
    sigevent_t      event;      // Signal/event to deliver on expiry
    thread_t        *owner;     // Process that owns this timer
    queue_node_t    node;       // Timer queue node
    queue_node_t    knode;      // Timer queue node in kernel timers list.
    spinlock_t      lock;
} posix_timer_t;

static inline int posix_timer_validate_clockid(clockid_t clockid) {
    return (
        clockid != CLOCK_REALTIME &&
        clockid != CLOCK_MONOTONIC &&
        clockid != CLOCK_PROCESS_CPUTIME_ID &&
        clockid != CLOCK_THREAD_CPUTIME_ID
    ) ? -EINVAL : 0;
}

#define POSIX_TIMER_ABSTIME 1

extern int timer_create(clockid_t clockid, sigevent_t *sevp, timer_t *timerid);
extern int timer_delete(timer_t timerid);
extern int timer_gettime(timer_t timerid, struct itimerspec *curr_value);
extern int timer_settime(timer_t timerid, int flags, const struct itimerspec *new_value, struct itimerspec *old_value);