#include <bits/errno.h>
#include <sys/_signal.h>
#include <sys/thread.h>

int sigmask(sigset_t *sigset, int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    int err = 0;

    if (sigset == NULL)
        return -EINVAL;

    if (oset)
        *oset = *sigset;

    if (set == NULL)
        return 0;

    if (sigismember(set, SIGKILL) || sigismember(set, SIGSTOP))
        return -EINVAL;

    switch (how) {
    case SIG_BLOCK:
        *sigset |= *set;
        break;
    case SIG_UNBLOCK:
        *sigset &= ~*set;
        break;
    case SIG_SETMASK:
        *sigset = *set;
        break;
    default:
        err = -EINVAL;
        break;
    }

    return err;
}

int pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    if (current == NULL)
        return -EINVAL;

    current_lock();
    int err = thread_sigmask(current, how, set, oset);
    current_unlock();
    return err;
}

int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    if (current == NULL)
        return -EINVAL;
    signal_lock(current->t_signals);
    int err = sigmask(&current->t_signals->sig_mask, how, set, oset);
    signal_unlock(current->t_signals);
    return err;
}
