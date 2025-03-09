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
        sched_wait(sigwaiters_queue, T_SLEEP, QUEUE_RELLOC_TAIL, NULL);
    }

    sigmask(&current->t_sigmask, SIG_SETMASK, &oset, NULL);

    return -1;
}