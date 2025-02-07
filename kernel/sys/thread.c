#include <core/debug.h>
#include <sys/thread.h>

const char *tget_state(tstate_t state) {
    static const char *states[] = {
        [T_EMBRYO]      = "T_EMBRYO",
        [T_READY]       = "T_READY",
        [T_RUNNING]     = "T_RUNNING",
        [T_SLEEP]       = "T_SLEEP",
        [T_STOPPED]     = "T_STOPPED",
        [T_ZOMBIE]      = "T_ZOMBIE",
        [T_TERMINATED]  = "T_TERMINATED",
    };
    if (state < T_EMBRYO || state > T_TERMINATED)
        return "UnknownState";
    return states[state];
}

tid_t thread_gettid(thread_t *thread) {
    return thread ? thread->t_info.ti_tid : 0;
}

pid_t thread_getpid(thread_t *thread) {
    return thread ? (thread->t_proc ? thread->t_proc->pid : 0) : 0;
}

tid_t gettid(void) {
    return thread_gettid(current);
}

pid_t getpid(void) {
    return thread_getpid(current);
}

tid_t thread_self(void) {
    return gettid();
}

int thread_schedule(thread_t *thread) {
    if (thread == NULL)
        return -EINVAL;
    todo("Under construction\n");
    return 0;
}

int thread_enqueue(queue_t *queue, thread_t *thread, queue_node_t **pnp) {
    int          err    = 0;

    if (queue == NULL || thread == NULL)
        return -EINVAL;

    thread_assert_locked(thread);

    queue_lock(&thread->t_queues);
    if ((err = enqueue(&thread->t_queues, queue, QUEUE_ENFORCE_UNIQUE, NULL))) {
        queue_unlock(&thread->t_queues);
        return err;
    }

    if ((err = enqueue(queue, thread, QUEUE_ENFORCE_UNIQUE, pnp))) {
        int Error;
        assert_eq(Error = queue_remove(&thread->t_queues, queue), 0,
            "queue_remove()[Err: %s]: Failed for thread[%d] following error: %s"
            "when enquing thread on queue\n",
            perror(Error), thread_gettid(thread), perror(err)
        );
        queue_unlock(&thread->t_queues);
        return err;
    }
    queue_unlock(&thread->t_queues);

    return 0;
}

int thread_remove_queue(thread_t *thread, queue_t *queue) {
    int err = 0;

    if (thread == NULL || queue == NULL)
        return -EINVAL;

    queue_assert_locked(queue);
    thread_assert_locked(thread);

    bool locked = queue_test_and_lock(&thread->t_queues);
    if ((err = queue_remove(queue, thread))) {
        goto error;
    }

    if ((err = queue_remove(&thread->t_queues, queue))) {
        int Err;
        assert_eq(Err = enqueue(queue, thread, QUEUE_ENFORCE_UNIQUE, NULL), 0,
            "enqueue():[Err: %s]: Failed to reverse queue_remove(queue, thread[%d])->err: %s\n",
            perror(Err), thread_gettid(thread), perror(err)
        );
        goto error;
    }

    if (locked)
        queue_unlock(&thread->t_queues);
    return 0;
error:
    if (locked)
        queue_unlock(&thread->t_queues);
    return err;
}

thread_t *thread_dequeue(queue_t *queue) {
    int err = 0;

    if (queue == NULL)
        return NULL;

    queue_foreach(queue, thread_t *, thread) {
        thread_lock(thread);
        queue_lock(&thread->t_queues);

        assert_eq(err = queue_remove(queue, thread), 0,
            "queue_remove()[Err: %s]: failed to remove thread[%d] from queue\n",
            perror(err), thread_gettid(thread)
        );

        assert_eq(err = queue_remove(&thread->t_queues, queue), 0,
            "queue_remove()[Err: %s]: failed to remove queue from thread[%d]\n",
            perror(err), thread_gettid(thread)
        );

        queue_unlock(&thread->t_queues);

        return thread;
    }

    return NULL;
}

int thread_queue_peek(queue_t *queue, tid_t tid, tstate_t state, thread_t **ptp) {
    if (queue == NULL)
        return -EINVAL;

    queue_foreach(queue, thread_t *, thread) {
        thread_lock(thread);

        if (thread_gettid(thread) == tid) { // found thread, peek it.
            if (ptp == NULL)
                thread_unlock(thread);
            *ptp = thread;
            return 0;
        } else if (tid == 0 && thread_in_state(thread, state)) { // peek any thread matching 'state'
            if (ptp == NULL)
                thread_unlock(thread);
            *ptp = thread;
            return 0;
        } else if (tid == -1) { // peek any thread.
            if (ptp == NULL)
                thread_unlock(thread);
            *ptp = thread;
            return 0;
        }

        thread_unlock(thread);
    }

    return -ESRCH;
}

int thread_create_group(thread_t *thread) {
    int         err      = 0;
    queue_t     *group   = NULL;
    cred_t      *cred    = NULL;
    file_ctx_t  *fctx    = NULL;
    sig_desc_t  *signals = NULL;

    if (thread == NULL)
        return -EINVAL;

    thread_assert_locked(thread);

    if (thread->t_group)
        return -EALREADY;

    if ((err = queue_alloc(&group)))
        return err;

    queue_lock(group);
    if ((err = thread_enqueue(group, thread, NULL))) {
        queue_unlock(group);
        goto error;
    }
    queue_unlock(group);

    thread_setmain(thread);

    thread->t_fctx          = fctx;
    thread->t_cred          = cred;
    thread->t_group         = group;
    thread->t_signals       = signals;
    thread->t_info.ti_tgid  = thread_gettid(thread);

    return 0;
error:
    if (group) queue_free(group);
    if (cred) { todo("FIXME!\n"); }
    if (fctx) { todo("FIXME!\n"); }
    if (signals) { todo("FIXME!\n"); }
    return err;
}

int thread_join_group(thread_t *other, thread_t *thread) {
    int err = 0;

    if (other == NULL || thread == NULL)
        return -EINVAL;

    queue_assert_locked(other->t_group);
    thread_assert_locked(thread);
    
    if ((err = thread_enqueue(other->t_group, thread, NULL)))
        return err;

    thread->t_proc        = other->t_proc;
    thread->t_mmap        = other->t_mmap;
    thread->t_cred        = other->t_cred;
    thread->t_fctx        = other->t_fctx;
    thread->t_group       = other->t_group;
    thread->t_signals     = other->t_signals;
    thread->t_info.ti_tgid= other->t_info.ti_tgid;

    return 0;
}

void thread_info_dump(thread_info_t *info) {
    printk(
        "ti_tid:        %d\n"
        "ti_ktid:       %d\n"
        "ti_tgid:       %d\n"
        "ti_entry:      %p\n"
        "ti_state:      %s\n"
        "ti_errno:      %d\n"
        "ti_flags:      %lX\n"
        "ti_exit_code:  %lX\n",
        info->ti_tid,
        info->ti_ktid,
        info->ti_tgid,
        info->ti_entry,
        tget_state(info->ti_state),
        info->ti_errno,
        info->ti_flags,
        info->ti_exit_code
    );
}