#include <sys/thread.h>
#include <core/debug.h>

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

    // Detach thread from all queues
    if (thread->t_global_qnode.queue)
        embedded_queue_remove(thread->t_global_qnode.queue, &thread->t_global_qnode);

    if (thread->t_group_qnode.queue)
        embedded_queue_remove(thread->t_group_qnode.queue, &thread->t_group_qnode);

    if (thread->t_run_qnode.queue)
        embedded_queue_remove(thread->t_run_qnode.queue, &thread->t_run_qnode);

    if (thread->t_wait_qnode.queue)
        embedded_queue_remove(thread->t_wait_qnode.queue, &thread->t_wait_qnode);

    todo("'Implement me :)'\n");
    return 0;
}