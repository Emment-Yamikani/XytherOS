#include <bits/errno.h>
#include <core/debug.h>
#include <sys/thread.h>

int thread_sigmask(thread_t *thread, int how, const sigset_t *restrict set, sigset_t *restrict oset) {
    if (thread == NULL)
        return -EINVAL;
    thread_assert_locked(thread);
    return sigmask(&thread->t_sigmask, how, set, oset);
}