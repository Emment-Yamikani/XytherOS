#include <bits/errno.h>
#include <core/debug.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/_wait.h>

int abandon_children(proc_t *target) {
    if (!curproc || !target || curproc == target) {
        return -EINVAL;
    }

    proc_assert_locked(target);

    queue_lock(&curproc->children);

    while (!queue_trylock(&target->children)) {
        queue_unlock(&curproc->children);
        hpet_milliwait(10);
        queue_lock(&curproc->children);
    }

    int err = 0;
    proc_t *child;
    queue_foreach_entry(&curproc->children, child, child_qnode) {
        proc_lock(child);
        err = embedded_queue_relloc(&target->children, &curproc->children,
            &child->child_qnode, QUEUE_UNIQUE, QUEUE_TAIL);
        proc_unlock(child);

        if (err != 0) return err;
    }

    queue_unlock(&target->children);
    queue_unlock(&curproc->children);

    return err;
}

void discard_signals(void) {
    signal_lock(current->t_signals);
    signal_reset(current->t_signals);
    signal_unlock(current->t_signals);

    current_lock();
    thread_reset_signal(current);
    current_unlock();
}

void signal_parent(void) {
    if (curproc->parent) {
        proc_lock(curproc->parent);
        cond_signal(&curproc->parent->child_event);
        proc_send_signal(curproc->parent, SIGCHLD, (sigval_t) {0});
        proc_unlock(curproc->parent);
    }
}

__noreturn void exit(int status) {
    assert_ne(curproc, initproc, "'init process' must not exit, (atleast for now).\n");

    // First all threads except 'current' MUST be killed.
    int err = thread_kill_others();
    assert_eq(err, 0, "Error: %s: failed to kill other threads.\n", strerror(err));

    // discard all signals.
    discard_signals();

    file_close_all();

    proc_lock(initproc);
    proc_lock(curproc);

    err = abandon_children(initproc);
    assert_eq(err, 0, "Error[%s]: Failed to abandon children.\n", strerror(err));

    proc_unlock(initproc);

    curproc->state  = P_TERMINATED;
    curproc->status = __W_EXITCODE(status, 0);

    // TODO: Maybe this should be done by the parent.
    mmap_lock(curproc->mmap);
    err = mmap_clean(curproc->mmap);
    mmap_unlock(curproc->mmap);
    assert_eq(err, 0, "Error[%s]: Failed to clean memory map.\n", strerror(err));

    signal_parent();

    proc_unlock(curproc);

    thread_exit(status);

    __builtin_unreachable();
}