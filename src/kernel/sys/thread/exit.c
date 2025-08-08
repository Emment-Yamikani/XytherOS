#include <bits/errno.h>
#include <core/debug.h>
#include <sys/thread.h>

void exit(int status __unused) {
    thread_t *thread;

    queue_lock(current->t_group);
    foreach_thread(current->t_group, thread, t_group_qnode) {
        if (current == thread) {
            continue;
        }

        thread_lock(thread);
        thread_kill(thread, NULL, NULL);
        thread_unlock(thread);
    }
    queue_unlock(current->t_group);

    todo();
}