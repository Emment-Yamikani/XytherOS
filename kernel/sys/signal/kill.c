#include <bits/errno.h>
#include <sys/_signal.h>
#include <sys/thread.h>

int pthread_sigqueue(tid_t tid, int signo, union sigval sigval) {
    int      err;
    thread_t *thread = NULL;

    if (SIGBAD(signo))
        return -EINVAL;

    queue_lock(global_thread_queue);
    if ((err = thread_queue_get_thread(global_thread_queue, tid, 0, &thread))) {
        queue_unlock(global_thread_queue);
        return err;
    }
    queue_unlock(global_thread_queue);

    err = thread_kill(thread, signo, sigval);
    thread_unlock(thread);

    return err;
}

int pthread_kill(tid_t tid, int signo) {
    return pthread_sigqueue(tid, signo, (union sigval){0});
}
