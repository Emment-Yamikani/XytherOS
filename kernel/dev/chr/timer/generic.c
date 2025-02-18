#include <bits/errno.h>
#include <core/debug.h>
#include <dev/timer.h>
#include <sync/atomic.h>
#include <sys/schedule.h>
#include <sys/_time.h>

const ulong SYS_Hz = 1000;

static volatile bool        use_hpet    = 0;
static volatile _Atomic(jiffies_t)   jiffies_now = 0; 

void jiffies_update(void) {
    atomic_inc(&jiffies_now);
}

jiffies_t jiffies_get(void) {
    return atomic_read(&jiffies_now);
}

void jiffies_timed_wait(jiffies_t jiffies) {
    jiffies_t jiffy = jiffies_get() + jiffies;
    while (time_before(jiffies_get(), jiffy));
}

jiffies_t jiffies_sleep(jiffies_t jiffies);

void jiffies_to_timespec(u64 jiffies, struct timespec *ts) {
    ts->tv_sec = jiffies / SYS_Hz;
    ts->tv_nsec= (jiffies % SYS_Hz) * 1000000;
}

u64 jiffies_from_timespec(const struct timespec *ts) {
    return (ts->tv_sec * SYS_Hz) + (ts->tv_nsec / 1000000);
}

int jiffies_getres(struct timespec *res) {
    if (res == NULL)
        return -EINVAL;
    
    res->tv_sec = 0;
    res->tv_nsec= 1000000000l / SYS_Hz;
    return 0;
}

int jiffies_gettime(struct timespec *tp) {
    if (tp == NULL)
        return -EINVAL;
    jiffies_to_timespec(jiffies_get(), tp);
    return 0;
}

void timer_wait(timerid_t timer, double sec) {
    switch (timer) {
        case TIMER_PIT: pit_wait(sec); break;
        case TIMER_RTC:
        case TIMER_HPET: hpet_wait(sec); break;
        break;
    }
}

int timer_init(void) {
    int err = 0;
    if ((err = hpet_init())) {
        debug("Error[%s]: HPET init failed. Fallback to PIT\n", perror(err));
        pit_init();
        return 0;
    }

    atomic_write(&use_hpet, true);
    return 0;
}

void timer_intr(void) {
    if (use_hpet)
        hpet_intr();
    else pit_intr();
}