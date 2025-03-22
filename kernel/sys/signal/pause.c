#include <bits/errno.h>
#include <core/timer.h>
#include <sys/schedule.h>
#include <sys/thread.h>

uint alarm (uint sec) {
    struct itimerspec it, rem;
    it.it_value     = (timespec_t){sec, 0};
    it.it_interval  = (timespec_t){0};

    timer_settime(current->t_alarm, 0, &it, &rem);
    return rem.it_value.tv_sec;
}

int pause(void) {
    if (!current) {
        return -EINVAL;
    }

    current_lock();
    while (signal_check_pending()) {
        current_unlock();
        sched_yield();
        current_lock();
    }
    current_unlock();

    return -EINTR;
}