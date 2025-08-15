#include <bits/errno.h>
#include <core/debug.h>
#include <sys/_signal.h>
#include <sys/schedule.h>
#include <sys/proc.h>
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

    return sigsetadd(&thread->t_sigpending, siginfo->si_signo);
}

int thread_send_signal(thread_t *thread, int signo, union sigval value) {
    if (!thread || SIGBAD(signo)) {
        return -EINVAL;
    }

    thread_assert_locked(thread);

    int  err;
    siginfo_t   *siginfo;
    if ((err = siginfo_create(signo, value, &siginfo))) {
        return err;
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

int signal_desc_send_signal(signal_t *signals, signo_t signo, union sigval sigval) {
    if (!signals || SIGBAD(signo)) {
        return -EINVAL;
    }

    signal_assert_locked(signals);

    siginfo_t *siginfo;
    int err = siginfo_create(signo, sigval, &siginfo);
    if (err) {
        return err;
    }

    err = signal_enqueue(signals, siginfo);

    if (err) {
        siginfo_free(siginfo);
    }

    return err;
}

int proc_send_signal(proc_t *proc, signo_t signo, sigval_t sigval) {
    if (!proc || SIGBAD(signo)) {
        return -EINVAL;
    }

    proc_assert_locked(proc);

    signal_lock(proc->signals);
    int err = signal_desc_send_signal(proc->signals, signo, sigval);
    signal_unlock(proc->signals);

    return err;
}

int kill(pid_t pid __unused, int signo __unused) {

    return -ENOSYS;
}