#include <core/debug.h>
#include <mm/kalloc.h>
#include <sys/schedule.h>
#include <sys/thread.h>

QUEUE(global_thread_queue);

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

const char *get_tstate(void) {
    return tget_state(current_get_state());
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

int thread_get_prio(thread_t *thread) {
    if (thread == NULL)
        return -EINVAL;
    
    thread_assert_locked(thread);

    return thread->t_info.ti_sched.ts_prio;
}

int thread_set_prio(thread_t *thread, int prio) {
    if ((thread == NULL) || (prio < MLFQ_LOW) || (prio > MLFQ_HIGH))
        return -EINVAL;

    thread_assert_locked(thread);
    thread->t_info.ti_sched.ts_prio = prio;
    return 0;
}

int thread_schedule(thread_t *thread) {
    if (thread == NULL)
        return -EINVAL;
    return sched_enqueue(thread);
}

int thread_get_info(tid_t tid, thread_info_t *ip) {
    if (ip == NULL)
        return -EINVAL;

    queue_lock(global_thread_queue);

    embedded_queue_foreach(global_thread_queue, thread_t, thread, t_global_qnode) {
        thread_lock(thread);
        if (thread_gettid(thread) == tid) {
            *ip = thread->t_info;
            thread_unlock(thread);
            queue_unlock(global_thread_queue);
            return 0;
        }
        thread_unlock(thread);
    }
    queue_unlock(global_thread_queue);

    return -ESRCH;
}

void thread_info_dump(thread_info_t *info) {
    printk(
        "\n++   Thread Info Dump   ++\n"
        "+-------------------------+\n"
        "  ti_tid:        %8d |\n"
        "  ti_ktid:       %8d |\n"
        "  ti_tgid:       %8d |\n"
        "  ti_entry:      %8X |\n"
        "  ti_state:      %8s |\n"
        "  ti_errno:      %8d |\n"
        "  ti_flags:      %8lX |\n"
        "  ti_exit_code:  %8lX |\n"
        "+-------------------------+\n",
        info->ti_tid,
        info->ti_ktid,
        info->ti_tgid,
        V2LO(info->ti_entry),
        tget_state(info->ti_state),
        info->ti_errno,
        info->ti_flags,
        info->ti_exit
    );
}

int thread_create_group(thread_t *thread) {
    int             err      = 0;
    cred_t          *cred    = NULL;
    file_ctx_t      *fctx    = NULL;
    queue_t         *queue   = NULL;
    signal_t        *signals = NULL;

    if (thread == NULL)
        return -EINVAL;

    thread_assert_locked(thread);

    if (thread->t_group)
        return -EALREADY;

    if ((err = queue_alloc(&queue)))
        return err;

    if ((err = queue_init(queue))) {
        goto error;
    }

    if ((err = signal_alloc(&signals))) {
        goto error;
    }

    queue_lock(queue);
    if ((err = embedded_enqueue(queue, &thread->t_group_qnode, QUEUE_UNIQUE))) {
        queue_unlock(queue);
        goto error;    
    }
    queue_unlock(queue);


    thread_setmain(thread);

    thread->t_fctx          = fctx;
    thread->t_cred          = cred;
    thread->t_group         = queue;
    thread->t_signals       = signals;
    thread->t_info.ti_tgid  = thread_gettid(thread);

    return 0;
error:
    if (queue) queue_free(queue);
    if (cred) {
        todo("FIXME!\n"); }
    if (fctx) {
        todo("FIXME!\n"); }
    if (signals) signal_free(signals);
    return err;
}

int thread_join_group(thread_t *thread) {
    int err = 0;

    if (thread == NULL || current == NULL)
        return -EINVAL;

    thread_assert_locked(thread);

    queue_lock(current->t_group);
    if ((err = embedded_enqueue(current->t_group, &thread->t_group_qnode, QUEUE_UNIQUE))) {
        queue_unlock(current->t_group);
        return err;
    }
    queue_unlock(current->t_group);

    thread->t_proc        = current->t_proc;
    thread->t_mmap        = current->t_mmap;
    thread->t_cred        = current->t_cred;
    thread->t_fctx        = current->t_fctx;
    thread->t_group       = current->t_group;
    thread->t_signals     = current->t_signals;
    thread->t_info.ti_tgid= current->t_info.ti_tgid;

    return 0;
}

int thread_builtin_init(void) {

    foreach_builtin_thread() {
        int err = thread_create(NULL, 
            thread->thread_entry,
            thread->thread_arg,
            THREAD_CREATE_SCHED,
            NULL
        );
        if (err) return err;
    }

    return 0;
}

int thread_queue_get_thread(queue_t *queue, tid_t tid, tstate_t state, thread_t **ptp) {
    if (queue == NULL)
        return -EINVAL;

    queue_foreach(queue, thread_t *, thread) {
        if (thread == current)
            continue;

        thread_lock(thread);

        if ((tid == 0) && thread_in_state(thread, state)) {
            if (ptp) *ptp = thread;
            else thread_unlock(thread);
            return 0;
        } else if ((thread_gettid(thread) == tid) || (tid == 0)) {
            if (ptp) *ptp = thread;
            else thread_unlock(thread);
            return 0;
        }

        thread_unlock(thread);
    }

    return -ESRCH;
}

int thread_wait(thread_t *thread) {
    int err = 0;

    if (thread == NULL)
        return -EINVAL;
    
    if (thread == current)
        return -EDEADLK;

    if (thread_iszombie(thread) || thread_isterminated(thread))
        return 0;

    while (!thread_in_state(thread, T_ZOMBIE)) {
        if ((err = cond_wait_releasing(&thread->t_event, &thread->t_lock))) {
            return err;
        }
    }

    return 0;
}

int thread_join(tid_t tid, thread_info_t *info, void **prp) {
    int         err = 0;
    thread_t    *thread = NULL;

    if (tid == gettid())
        return -EDEADLK;

    queue_lock(current->t_group);
    
    if ((err = thread_queue_get_thread(current->t_group, tid, T_ZOMBIE, &thread))) {
        queue_unlock(current->t_group);
        return err;
    }

    queue_unlock(current->t_group);

    if ((err = thread_wait(thread))) {
        thread_unlock(thread);
        return err;
    }

    // If we get here, target thread has entered T_ZOMBIE state.
    // Clean ttarget thread.
    return thread_reap(thread, info, prp);
}