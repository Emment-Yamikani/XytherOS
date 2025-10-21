#include <bits/errno.h>
#include <core/debug.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/_wait.h>

pid_t match_child_pid(pid_t pid, int *stat, int opt) {
    proc_t *child = NULL;
    queue_lock(&curproc->children);
    queue_foreach_entry(&curproc->children, child, child_qnode) {
        proc_lock(child);
        if (pid == child->pid) {
            break;
        }
        proc_unlock(child);
    }
    queue_unlock(&curproc->children);

    if (child == NULL) {
        return -ECHILD;
    }

    int retval = 0;
    while (!__proc_died(child) && !__proc_stopped(child)) {
        if (opt & WNOHANG) {
            proc_unlock(child);
            return 0;
        }

        proc_unlock(child);
        retval = cond_wait(&curproc->child_event, NULL, &curproc->lock);
        proc_lock(child);
        if (retval) {
            break;
        }
    }

    if (stat && (__proc_died(child) || __proc_stopped(child))) {
        *stat = child->status;
    }

    if (__proc_died(child)) {
        proc_free(child);
    } else {
        proc_unlock(child);
    }

    return pid;
}

pid_t match_pid(int *stat, int opt) {
    loop() {
        proc_t *child;
        queue_lock(&curproc->children);
        queue_foreach_entry(&curproc->children, child, child_qnode) {
            proc_lock(child);
            if (__proc_died(child) || __proc_stopped(child)) {
                if (stat) {
                    *stat = child->status;
                }

                pid_t pid = child->pid;
                if (__proc_died(child)) {
                    proc_free(child);
                } else {
                    proc_unlock(child);
                }

                queue_unlock(&curproc->children);
                return pid;
            }
            proc_unlock(child);
        }
        queue_unlock(&curproc->children);

        if (opt & WNOHANG) {
            return -ECHILD;
        }

        int err;
        if ((err = cond_wait(&curproc->child_event, NULL, &curproc->lock))) {
            return err;
        }
    }
}

pid_t match_pgid(pid_t pgid, int *stat, int opt) {
    loop() {
        proc_t *child;
        queue_lock(&curproc->children);
        queue_foreach_entry(&curproc->children, child, child_qnode) {
            proc_lock(child);
            if (pgid == child->pgid) {
                if (__proc_died(child) || __proc_stopped(child)) {
                    if (stat) {
                        *stat = child->status;
                    }

                    pid_t pid = child->pid;
                    if (__proc_died(child)) {
                        proc_free(child);
                    } else {
                        proc_unlock(child);
                    }

                    queue_unlock(&curproc->children);
                    return pid;
                }
            }
            proc_unlock(child);
        }
        queue_unlock(&curproc->children);

        if (opt & WNOHANG) {
            return -ECHILD;
        }

        int err;
        if ((err = cond_wait(&curproc->child_event, NULL, &curproc->lock))) {
            return err;
        }
    }
}

pid_t waitpid(pid_t pid, int *stat, int opt) {
    if (pid == getpid()) {
        return -EDEADLOCK;
    }

    int retval = -ECHILD;
    proc_lock(curproc);

    if (pid < -1) {
        retval = match_pgid(ABS(pid), stat, opt);
    } else if (pid == -1) {
        retval = match_pid(stat, opt);
    } else if (pid == 0) {
        retval = match_pgid(curproc->pgid, stat, opt);
    } else {
        retval = match_child_pid(pid, stat, opt);
    }

    proc_unlock(curproc);
    return retval;
}