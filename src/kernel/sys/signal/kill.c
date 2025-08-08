#include <bits/errno.h>
#include <core/debug.h>
#include <sys/_signal.h>
#include <sys/schedule.h>
#include <sys/thread.h>

int thread_send_siginfo(thread_t *thread, siginfo_t *siginfo) {
    if (!thread || !siginfo) {
        return -EINVAL;
    }
    
    thread_assert_locked(thread);
    
    queue_lock(&thread->t_sigqueue[siginfo->si_signo - 1]);
    int err = sigqueue_enqueue(&thread->t_sigqueue[siginfo->si_signo - 1], siginfo);
    queue_unlock(&thread->t_sigqueue[siginfo->si_signo - 1]);

    if (err) {
        return err;
    }

    if ((err = thread_wakeup(thread, WAKEUP_SIGNAL))) {
        return err;
    }

    sigsetadd(&thread->t_sigpending, siginfo->si_signo);
    return 0;
}

int thread_send_signal(thread_t *thread, int signo, union sigval value) {
    int         err;
    siginfo_t   *siginfo = NULL;

    if (!thread || SIGBAD(signo)) {
        return -EINVAL;
    }

    thread_assert_locked(thread);

    if ((err = siginfo_alloc(&siginfo))) {
        return err;
    }

    if ((err = siginfo_init(siginfo, signo, value))) {
        goto error;
    }

    if ((err = thread_send_siginfo(thread, siginfo))) {
        goto error;
    }

    return 0;
error:
    if (siginfo) {
        siginfo_free(siginfo);
    }

    return err;
}

int pthread_sigqueue(tid_t tid, int signo, union sigval sigval) {
    int      err;
    thread_t *thread = NULL;

    if (SIGBAD(signo)) {
        return -EINVAL;
    }

    if ((err = thread_group_get_by_tid(tid, &thread))) {
        return err;
    }

    err = thread_send_signal(thread, signo, sigval);
    thread_unlock(thread);
    return err;
}

int pthread_kill(tid_t tid, int signo) {
    return pthread_sigqueue(tid, signo, (union sigval){0});
}

int kill(pid_t pid __unused, int signo __unused) {

    return -ENOSYS;
}