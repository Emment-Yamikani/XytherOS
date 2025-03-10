#include <bits/errno.h>
#include <sys/_signal.h>
#include <sys/thread.h>

static int do_sigmask(sigset_t *sigset, int how, const sigset_t *set) {
    if (how == SIG_SETMASK) {
        *sigset = *set;
    } else if (how == SIG_BLOCK) {
        for (usize i = 0; i < __NR_INT; ++i) {
            sigset->sigset[i] |= set->sigset[i];
        }
    } else if (how == SIG_UNBLOCK) {
        for (usize i = 0; i < __NR_INT; ++i) {
            sigset->sigset[i] &= ~set->sigset[i];
        }
    } else return -EINVAL;
    
    return 0;
}

int sigmask(sigset_t *sigset, int how, const sigset_t *set, sigset_t *oset) {
    if (!sigset) return -EINVAL;

    if (oset) *oset = *sigset;

    if (!set) return 0;

    if (sigismember(set, SIGKILL) || sigismember(set, SIGSTOP)) {
        return -EINVAL;
    }

    return do_sigmask(sigset, how, set);
}

int thread_sigmask(thread_t *thread, int how, const sigset_t *set, sigset_t *oset) {
    if (thread == NULL)
        return -EINVAL;
    thread_assert_locked(thread);
    return sigmask(&thread->t_sigmask, how, set, oset);
}

int pthread_sigmask(int how, const sigset_t *set, sigset_t *oset) {
    if (current == NULL)
        return -EINVAL;

    current_lock();
    int err = thread_sigmask(current, how, set, oset);
    current_unlock();
    return err;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oset) {
    if (current == NULL)
        return -EINVAL;
    signal_lock(current->t_signals);
    int err = sigmask(&current->t_signals->sig_mask, how, set, oset);
    signal_unlock(current->t_signals);
    return err;
}
