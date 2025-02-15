#include <core/debug.h>
#include <mm/kalloc.h>
#include <sys/schedule.h>
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
        info->ti_exit
    );
}

int thread_create_group(thread_t *thread) {
    int             err      = 0;
    cred_t          *cred    = NULL;
    file_ctx_t      *fctx    = NULL;
    sig_desc_t      *signals = NULL;
    queue_t         *queue   = NULL;

    if (thread == NULL)
        return -EINVAL;

    thread_assert_locked(thread);

    if (thread->t_group)
        return -EALREADY;

    if (NULL == (queue = kmalloc(sizeof *queue)))
        return -ENOMEM;

    if ((err = queue_init(queue))) {
        goto error;
    }

    queue_lock(queue);
    if ((err = embedded_enqueue(queue, &thread->t_group_qnode, QUEUE_ENFORCE_UNIQUE))) {
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
    if (queue) kfree(queue);
    if (cred) {
        todo("FIXME!\n"); }
    if (fctx) {
        todo("FIXME!\n"); }
    if (signals) {
        todo("FIXME!\n"); }
    return err;
}

int thread_join_group(thread_t *thread) {
    int err = 0;

    if (thread == NULL || current == NULL)
        return -EINVAL;

    thread_assert_locked(thread);

    queue_lock(current->t_group);
    if ((err = embedded_enqueue(current->t_group, &thread->t_group_qnode, QUEUE_ENFORCE_UNIQUE))) {
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
    int err = 0;

    foreach_builtin_thread() {
        if ((err = thread_create(NULL, thread->thread_entry, thread->thread_arg, THREAD_CREATE_SCHED, NULL)))
            return err;
    }

    return 0;
}