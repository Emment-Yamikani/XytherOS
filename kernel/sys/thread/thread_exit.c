#include <core/debug.h>
#include <sys/schedule.h>
#include <sys/thread.h>

void thread_exit(uintptr_t exit_code) {
    current_assert();
    arch_thread_exit(exit_code);
}

int thread_reap(thread_t *thread, thread_info_t *info, void **retval) {

    if (thread == NULL)
        return -EINVAL;

    thread_assert_locked(thread);

    if (info != NULL) { // copy the thread info.
        info->ti_tid    = thread->t_info.ti_tid;
        info->ti_ktid   = thread->t_info.ti_ktid;
        info->ti_tgid   = thread->t_info.ti_tgid;
        info->ti_entry  = thread->t_info.ti_entry;
        info->ti_state  = thread->t_info.ti_state;
        info->ti_sched  = thread->t_info.ti_sched;
        info->ti_errno  = thread->t_info.ti_errno;
        info->ti_flags  = thread->t_info.ti_flags;
        info->ti_exit   = thread->t_info.ti_exit;
    }

    //get the return value.
    if (retval) *retval = (void *)thread->t_info.ti_exit;

    if (thread_isdetached(thread)) {
        thread_unlock(thread);
        return 0;
    }

    thread_free(thread);
    return 0;
}

int thread_cancel(tid_t tid) {
    int         err     = 0;

    if (tid <= 0)
        return -EINVAL;

try:
    queue_lock(current->t_group);
    embedded_queue_foreach(current->t_group, thread_t, thread, t_group_qnode) {
        thread_lock(thread);

        if (thread_gettid(thread) == tid) {

            if (thread->t_wait_queue) {
                queue_t *wait_queue = thread->t_wait_queue;

                if (!queue_trylock(wait_queue)) {
                    thread_unlock(thread);
                    queue_unlock(current->t_group);
                    goto try;
                }

                if ((err = sched_detach_and_wakeup(wait_queue, thread))) {
                    thread_unlock(thread);
                    queue_unlock(current->t_group);
                    queue_unlock(wait_queue);
                    return err;
                }

                queue_unlock(wait_queue);
            }

            thread_set(thread, THREAD_CANCEL);
            thread->t_info.ti_ktid = gettid();

            thread_unlock(thread);
            queue_unlock(current->t_group);
            return 0;
        }

        thread_unlock(thread);
    }

    queue_unlock(current->t_group);
    return -ESRCH;
}