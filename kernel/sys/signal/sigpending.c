#include <bits/errno.h>
#include <sys/thread.h>

static int thread_sigpending(thread_t *thread, sigset_t *set) {
    int npending = 0;

    if (thread == NULL || set == NULL)
        return -EINVAL;
    
    thread_assert_locked(thread);

    signal_lock(thread->t_signals);
    for (int signo = 0; signo < NSIG; ++signo) {
        if (sigismember(&thread->t_sigpending, signo + 1) ||
            sigismember(&thread->t_signals->sigpending, signo + 1)) {
            if (sigismember(&thread->t_sigmask, signo + 1)) {
                sigaddset(set, signo + 1);
                npending++;
            }
        }
    }
    signal_unlock(thread->t_signals);

    return npending > 0 ? 0 : -1;
}

int sigpending(sigset_t *set) {
    if (current == NULL || set == NULL)
        return -EINVAL;

    current_lock();
    int err = thread_sigpending(current, set);
    current_unlock();
    return err;
}