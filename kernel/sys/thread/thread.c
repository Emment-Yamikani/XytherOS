#include <core/debug.h>
#include <mm/kalloc.h>
#include <sys/schedule.h>
#include <sys/thread.h>

QUEUE(global_thread_queue);

const char *tget_state(tstate_t state) {
    static const char *states[] = {
        [T_EMBRYO]      = "EMBRYO",
        [T_READY]       = "READY",
        [T_RUNNING]     = "RUNNING",
        [T_SLEEP]       = "SLEEP",
        [T_STOPPED]     = "STOPPED",
        [T_ZOMBIE]      = "ZOMBIE",
        [T_TERMINATED]  = "TERMINATED",
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
    if (thread == NULL) {
        return -EINVAL;
    }
    return sched_enqueue(thread);
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
    queue_t         *timers  = NULL;
    signal_t        *signals = NULL;

    if (thread == NULL)
        return -EINVAL;

    thread_assert_locked(thread);

    if (thread->t_group)
        return -EALREADY;

    if ((err = queue_alloc(&queue)))
        return err;

    if ((err = queue_alloc(&timers))) {
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

    thread->t_timers        = timers;
    thread->t_fctx          = fctx;
    thread->t_cred          = cred;
    thread->t_group         = queue;
    thread->t_signals       = signals;
    thread->t_info.ti_tgid  = thread_gettid(thread);

    if ((err = timer_create_r(thread, CLOCK_REALTIME, NULL, &thread->t_alarm))) {
        goto error;
    }

    return 0;
error:
    if (queue) queue_free(queue);
    if (timers) queue_free(timers);

    if (cred) {
        todo("FIXME!\n"); }
    if (fctx) {
        todo("FIXME!\n"); }
    if (signals) signal_free(signals);
    return err;
}

int thread_join_group(thread_t *peer, thread_t *thread) {
    int err = 0;

    if (thread == NULL || peer == NULL) {
        return -EINVAL;
    }

    thread_assert_locked(thread);

    queue_lock(peer->t_group);
    if ((err = embedded_enqueue(peer->t_group, &thread->t_group_qnode, QUEUE_UNIQUE))) {
        queue_unlock(peer->t_group);
        return err;
    }
    queue_unlock(peer->t_group);

    thread->t_alarm       = peer->t_alarm;
    thread->t_proc        = peer->t_proc;
    thread->t_mmap        = peer->t_mmap;
    thread->t_cred        = peer->t_cred;
    thread->t_fctx        = peer->t_fctx;
    thread->t_group       = peer->t_group;
    thread->t_signals     = peer->t_signals;
    thread->t_info.ti_tgid= peer->t_info.ti_tgid;

    return 0;
}

int thread_builtin_init(void) {
    foreach_builtin_thread() {
        int err = thread_create(
            NULL, 
            thread->thread_entry,
            thread->thread_arg,
            THREAD_CREATE_SCHED,
            NULL
        );

        if (err) {
            return err;
        }
    }

    return 0;
}

int thread_queue_find_by_tid(queue_t *thread_queue, tid_t tid, thread_t **ptp) {
    if (thread_queue == NULL) {
        return -EINVAL;
    }
    
    queue_foreach(thread_queue, thread_t *, thread) {
        thread_lock(thread);
        if (thread_gettid(thread) == tid) {
            if (ptp == NULL) {
                thread_unlock(thread);
            } else {
                *ptp = thread;
            }
            return 0;
        }
        thread_unlock(thread);
    }

    return -ESRCH;
}

int thread_group_get_by_tid(tid_t tid, thread_t **ptp) {
    if (ptp == NULL) {
        return -EINVAL;
    }

    queue_lock(current->t_group);
    int err = thread_queue_find_by_tid(current->t_group, tid, ptp);
    queue_unlock(current->t_group);

    return err;
}

int global_find_by_tid(tid_t tid, thread_t **ptp) {
    if (!ptp) {
        return -EINVAL;
    }

    queue_lock(global_thread_queue);
    int err = thread_queue_find_by_tid(global_thread_queue, tid, ptp);
    queue_unlock(global_thread_queue);
    return err;
}

int enqueue_global_thread(thread_t *thread) {
    if (thread == NULL) {
        return -EINVAL;
    }

    queue_lock(global_thread_queue);
    int err = thread_enqueue(global_thread_queue, thread, t_global_qnode, QUEUE_TAIL);
    queue_unlock(global_thread_queue);
    return err;
}

int thread_get_info(thread_t *thread, thread_info_t *info) {
    if (!thread || !info) {
        return -EINVAL;
    }

    thread_assert_locked(thread);

    *info = thread->t_info;

    return 0;
}

int thread_get_info_by_id(tid_t tid, thread_info_t *info) {
    thread_t *thread = NULL;

    if (!info) {
        return -EINVAL;
    }

    int err = thread_group_get_by_tid(tid, &thread);

    if (thread) {
        *info = thread->t_info;
        thread_unlock(thread);
    }

    return err;
}