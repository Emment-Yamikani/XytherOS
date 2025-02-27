#include <bits/errno.h>
#include <sys/thread.h>

void signal_dispatch(void) {
    siginfo_t *siginfo = NULL;

    if (!current)
        return;

    current_lock();
    if (signal_dequeue(current, &siginfo)) {
        current_unlock();
        return;
    }
    current_unlock();

    siginfo_dump(siginfo);
}