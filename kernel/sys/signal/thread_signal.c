#include <bits/errno.h>
#include <core/debug.h>
#include <sys/schedule.h>
#include <sys/thread.h>

int thread_sigmask(thread_t *thread, int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    if (thread == NULL)
        return -EINVAL;
    thread_assert_locked(thread);
    return sigmask(&thread->t_sigmask, how, set, oset);
}

int thread_sigsend(thread_t *thread, siginfo_t *siginfo) {
    int err;

    if (thread == NULL || siginfo == NULL)
        return -EINVAL;

    thread_assert_locked(thread);

    queue_lock(&thread->t_sigqueue[siginfo->si_signo - 1]);
    if ((err = sigqueue_enqueue(&thread->t_sigqueue[siginfo->si_signo - 1], siginfo))) {
        queue_unlock(&thread->t_sigqueue[siginfo->si_signo - 1]);
        return err;
    }

    if (thread_issleep(thread)) {
        queue_t *wait_queue = thread->t_wait_queue;
        queue_lock(wait_queue);
        if ((err = sched_detach_and_wakeup(wait_queue, thread))) {
            int Err = queue_remove(&thread->t_sigqueue[siginfo->si_signo - 1], siginfo);

            assert_eq(Err, 0, "Err[%s]: Failed to remove siginfo[%d] from thread[%d:%d].??\n",
                perror(Err), siginfo->si_signo, thread_getpid(thread), thread_gettid(thread));

            queue_unlock(wait_queue);
            queue_unlock(&thread->t_sigqueue[siginfo->si_signo - 1]);
            return err;
        }
        queue_unlock(wait_queue);
    }

    queue_unlock(&thread->t_sigqueue[siginfo->si_signo - 1]);
    return err;
}

int thread_kill(thread_t *thread, int signo, union sigval value) {
    int         err;
    siginfo_t   *siginfo = NULL;

    if (thread == NULL || SIGBAD(signo))
        return -EINVAL;

    thread_assert_locked(thread);

    if ((err = siginfo_alloc(&siginfo)))
        return err;

    if ((err = siginfo_init(siginfo, signo, value)))
        goto error;

    if ((err = thread_sigsend(thread, siginfo)))
        goto error;

    return 0;
error:
    if (siginfo) siginfo_free(siginfo);
    return err;
}