#include <core/debug.h>
#include <sys/schedule.h>
#include <sys/thread.h>

void thread_exit(uintptr_t status) {
    current_assert();
    arch_thread_exit(status);
}

int thread_reap(thread_t *thread, thread_info_t *info, void **retval) {
    if (thread == NULL) {
        return -EINVAL;
    }

    thread_assert_locked(thread);

    int err = 0;
    if ((err = thread_wait(thread))) {
        return err;
    }

    if (info != NULL) { // copy the thread info.
        info->ti_exit   = thread->t_info.ti_exit;
        info->ti_errno  = thread->t_info.ti_errno;
        info->ti_entry  = thread->t_info.ti_entry;

        info->ti_flags  = thread->t_info.ti_flags;
        
        info->ti_sched  = thread->t_info.ti_sched;
        info->ti_state  = thread->t_info.ti_state;
        
        info->ti_ktid   = thread->t_info.ti_ktid;
        info->ti_tid    = thread->t_info.ti_tid;
        info->ti_tgid   = thread->t_info.ti_tgid;
    }

    //get the return value.
    if (retval != NULL) {
        *retval = (void *)thread->t_info.ti_exit;
    }

    if (thread_is_detached(thread)) {
        thread_unlock(thread);
        return 0;
    }

    thread_free(thread);
    return 0;
}

int thread_kill(thread_t *thread, thread_info_t *info, void **retval) {
    if (thread == NULL) {
        return -EINVAL;
    }

    // thread_set_kill(thread);

    int err;
    if ((err = thread_send_signal(thread, SIGKILL, (sigval_t){0}))) {
        return err;
    }

    return thread_reap(thread, info, retval);
}

int thread_kill_others(void) {
    if (current == NULL) {
        return -EINVAL;
    }

    int err = 0;
    thread_t *thread;
    queue_lock(current->t_group);
    foreach_thread(current->t_group, thread, t_group_qnode) {
        if (current == thread) {
            continue;
        }

        thread_lock(thread);
        queue_unlock(current->t_group);
        err = thread_kill(thread, NULL, NULL);
        queue_lock(current->t_group);
        if (err) { // break if an error occurs.
            thread_unlock(thread);
            break;
        }

    }
    queue_unlock(current->t_group);

    return err;
}

int thread_cancel(tid_t tid) {
    if (tid <= 0 || !current) {
        return -EINVAL;
    }

    if (thread_match_tid(current, tid)) {
        return -EDEADLOCK;
    }

try: thread_t *thread = NULL;
    queue_lock(current->t_group);
    foreach_thread(current->t_group, thread, t_group_qnode) {
        thread_lock(thread);

        if (thread_match_tid(thread, tid)) {
            if (thread->t_wait_queue) {
                queue_t *wait_queue = thread->t_wait_queue;

                if (!queue_trylock(wait_queue)) {
                    thread_unlock(thread);
                    queue_unlock(current->t_group);
                    goto try;
                }

                int err = 0;
                if ((err = sched_detach_and_wakeup(wait_queue, thread, WAKEUP_SIGNAL))) {
                    thread_unlock(thread);
                    queue_unlock(current->t_group);
                    queue_unlock(wait_queue);
                    return err;
                }

                queue_unlock(wait_queue);
            }

            thread_set_canceled(thread);
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
