#include <sys/thread.h>
#include <core/debug.h>

void thread_exit(uintptr_t exit_code) {
    current_assert();
    arch_thread_exit(exit_code);
}

int thread_reap(thread_t *thread, thread_info_t *info, void **retval) {
    int err = 0;

    if (thread == NULL)
        return -EINVAL;

    thread_assert_locked(thread);

    // detach from associated queues.
    queue_lock(&thread->t_queues);
    queue_foreach(&thread->t_queues, queue_t *, queue) {
        queue_lock(queue);
         if ((err = queue_remove(queue, thread))) {
            queue_unlock(&thread->t_queues);
            return err;
        }

        if ((err = queue_remove(&thread->t_queues, queue))) {
            int Err;
            assert_eq(Err = enqueue(queue, thread, QUEUE_ENFORCE_UNIQUE, NULL), 0,
                "enqueue():[Err: %s]: Failed to reverse queue_remove(queue, thread[%d])->err: %s\n",
                perror(Err), thread_gettid(thread), perror(err)
            );
            queue_unlock(&thread->t_queues);
            return err;
        }
        queue_unlock(queue);
    }
    queue_unlock(&thread->t_queues);

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

    // todo("'Implement me :)'\n");
    return 0;
}