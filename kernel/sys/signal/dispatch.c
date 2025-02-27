#include <bits/errno.h>
#include <sys/thread.h>

void signal_dispatch(void) {
    siginfo_t *siginfo = NULL;

    if (signal_dequeue(current, &siginfo)) {
        return;
    }

    siginfo_dump(siginfo);
}