#include <bits/errno.h>
#include <core/debug.h>
#include <sys/schedule.h>
#include <sys/thread.h>

/* Helper to check the current thread’s signal queue.
 * Returns the signal number if one is found (and properly dequeued),
 * or 0 if none is pending. */
static int check_thread_signals(const sigset_t *pending, siginfo_t **siginfo) {
    int signo = 0;
    sigset_t out;
    sigsetempty(&out);

    current_lock();
    sigsettest(pending, &current->t_sigpending, &out);
    if ((signo = sigset_first(&out))) {
        queue_t *queue = &current->t_sigqueue[signo - 1];
        queue_lock(queue);
        if (!sigqueue_dequeue(queue, siginfo)) {
            if (!queue_length(queue)) {
                sigsetdel(&current->t_sigpending, signo);
            }
            queue_unlock(queue);
            current_unlock();
            return signo;
        }
        queue_unlock(queue);
    }
    current_unlock();
    return 0;
}

/* Helper to check the system’s signal queue.
 * Returns the signal number if one is found (and properly dequeued),
 * or 0 if none is pending. */
static int check_process_signals(const sigset_t *pending, siginfo_t **siginfo) {
    int signo = 0;
    sigset_t out;
    sigsetempty(&out);

    signal_lock(current->t_signals);
    sigsettest(pending, &current->t_signals->sigpending, &out);
    if ((signo = sigset_first(&out))) {
        queue_t *queue = &current->t_signals->sig_queue[signo - 1];
        queue_lock(queue);
        if (!sigqueue_dequeue(queue, siginfo)) {
            if (!queue_length(queue)) {
                sigsetdel(&current->t_signals->sigpending, signo);
            }
            queue_unlock(queue);
            signal_unlock(current->t_signals);
            return signo;
        }
        queue_unlock(queue);
    }
    signal_unlock(current->t_signals);
    return 0;
}

static signo_t check_signals(sigset_t *pending, siginfo_t **psiginfo) {
    signo_t signo;

    if ((signo = check_thread_signals(pending, psiginfo)) != 0) {
        return signo;
    }

    return check_process_signals(pending, psiginfo);
}

int sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout) {
    if (set == NULL) {
        return -EINVAL;
    }

    if (timeout && ((timeout->tv_nsec >= 1000000000L) || 
        (timeout->tv_sec < 0 || timeout->tv_nsec < 0))) {
        return -EINVAL;
    }

    int signo;
    sigset_t pending;
    siginfo_t *siginfo = NULL;

    sigsetempty(&pending);
    sigsetor(&pending, set, &pending);

    sigsetdelmask(&pending, SIGMASK(SIGKILL) | SIGMASK(SIGSTOP));

    if (timeout) {
        jiffies_t expr = jiffies_from_timespec(timeout) + jiffies_get();
        while (time_before(jiffies_get(), expr)) {
            if ((signo = check_signals(&pending, &siginfo))) {
                if (info) *info = *siginfo;
                siginfo_free(siginfo);
                return signo;
            }
            sched_yield();
        }
        return -EAGAIN;
    }

    /* Loop indefinitely if no timeout was specified. */
    while (1) {
        if ((signo = check_signals(&pending, &siginfo))) {
            if (info) *info = *siginfo;
            siginfo_free(siginfo);
            return signo;
        }
        sched_yield();
    }

    return -EAGAIN; // Not reached
}

int sigwaitinfo(const sigset_t *set, siginfo_t *info) {
    /* POSIX permits 'timeout' to be NULL, so pass NULL */
    return sigtimedwait(set, info, NULL);
}