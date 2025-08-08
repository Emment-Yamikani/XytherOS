#include <bits/errno.h>
#include <core/debug.h>
#include <sys/schedule.h>
#include <sys/thread.h>

QUEUE(sigwaiters_queue);

int sigsuspend(const sigset_t *mask) {
    int err;
    sigset_t oset;

    if (mask == NULL || current == NULL) {
        return -EINVAL;
    }

    sigsetempty(&oset);
    
    if ((err = sigmask(&current->t_sigmask, SIG_SETMASK, mask, &oset))) {
        return err;
    }

    
    while (signal_dispatch()) {
        if ((err = sched_wait_whence(sigwaiters_queue, T_SLEEP, QUEUE_TAIL, NULL))) {
            break;
        }
    }

    sigmask(&current->t_sigmask, SIG_SETMASK, &oset, NULL);

    return err;
}