#include <bits/errno.h>
#include <core/timer.h>
#include <core/debug.h>
#include <sys/schedule.h>
#include <sys/thread.h>

unsigned int alarm (unsigned int sec) {
    struct itimerspec rem;
    timer_settime(
        current->t_alarm, 0,
        &(struct itimerspec){
            (timespec_t){0, 0},
            (timespec_t){sec, 0}
        },
        &rem
    );
    return rem.it_value.tv_sec;
}

int pause(void) {
    if (!current) {
        return -EINVAL;
    }

    current_lock();
    thread_sleep(NULL);
    current_unlock();
    return -EINTR;
}