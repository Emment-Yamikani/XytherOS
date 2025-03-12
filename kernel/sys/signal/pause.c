#include <bits/errno.h>
#include <sys/schedule.h>
#include <sys/thread.h>

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