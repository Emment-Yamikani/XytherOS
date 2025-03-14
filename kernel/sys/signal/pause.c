#include <bits/errno.h>
#include <core/timer.h>
#include <sys/schedule.h>
#include <sys/thread.h>

unsigned int alarm(unsigned int seconds) {
    timer_t id;

    void send_sigalarm(void *target) {
        thread_t *thread = target;
 
        thread_lock(thread);
        thread_kill((thread_t *)thread, SIGALRM, (union sigval){0});
        thread_unlock(thread);
    }

    ktimer_create(jiffies_from_s(seconds), 0, send_sigalarm, current, &id);
    return 0;
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