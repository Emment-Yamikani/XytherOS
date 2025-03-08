#include <bits/errno.h>
#include <sys/_signal.h>
#include <sys/thread.h>

static void do_mask(sigset_t *sigset, const sigset_t *set) {
    for (usize i = 0; i < __NR_INT; ++i) {
        sigset->sigset[i] &= ~set->sigset[i];
    }
}

static void do_set(sigset_t *sigset, const sigset_t *set) {
    for (usize i = 0; i < __NR_INT; ++i) {
        sigset->sigset[i] |= set->sigset[i];
    }
}

int sigmask(sigset_t *sigset, int how, const sigset_t *set, sigset_t *oset) {
    if (!sigset) return -EINVAL;  // Early return

    if (oset) *oset = *sigset;

    if (!set) return 0;  // Early return

    if (sigismember(set, SIGKILL) || sigismember(set, SIGSTOP))
        return -EINVAL;  // Restriction check

    if (how == SIG_SETMASK) {
        *sigset = *set;
        return 0;
    } else if (how == SIG_BLOCK) {
        do_set(sigset, set);
        return 0;
    } else if (how == SIG_UNBLOCK) {
        do_mask(sigset, set);
        return 0;
    }
    
    return -EINVAL;
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
