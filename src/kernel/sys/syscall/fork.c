#include <bits/errno.h>
#include <core/debug.h>
#include <sys/proc.h>
#include <sys/thread.h>

int copy_proc(proc_t *child, proc_t *parent) {
    int         err     = 0;
    file_ctx_t *fctx    = NULL;
    cred_t      *cred   = NULL;

    if (child == NULL || parent == NULL) {
        return -EINVAL;
    }
    
    proc_assert_locked(child);
    proc_assert_locked(parent);

    if ((err = procQ_insert(child))) {
        return err;
    }

    mmap_lock(parent->mmap);
    mmap_lock(child->mmap);

    err = mmap_copy(child->mmap, parent->mmap);

    mmap_unlock(child->mmap);
    mmap_unlock(parent->mmap);

    if (err) goto error;

    fctx = current->t_fctx;
    cred = current->t_cred;

    fctx_lock(fctx);
    fctx_lock(child->fctx);

    err = file_copy(child->fctx, fctx);

    fctx_unlock(child->fctx);
    fctx_unlock(fctx);

    if (err) goto error;

    cred_lock(cred);
    cred_lock(child->cred);

    err = cred_copy(child->cred, cred);

    cred_unlock(child->cred);
    cred_unlock(cred);

    if (err) {
        goto error;
    }

    queue_lock(&parent->children);
    err = embedded_enqueue(&parent->children, &child->child_qnode, QUEUE_UNIQUE);
    queue_unlock(&parent->children);

    if (err) {
        goto error;
    }

    child->sid      = parent->sid;
    child->pgid     = parent->pgid;
    child->entry    = parent->entry;
    child->status   = parent->status;
    child->parent   = proc_getref(parent);

    return 0;
error:
    /// TODO: Reverse mmap_clone(),
    /// but i think it will be handled by proc_free().
    return err;
}

pid_t fork(void) {
    if (curproc == NULL) {
        return -EINVAL;
    }

    proc_t *child;
    proc_lock(curproc);
    int err = proc_alloc(curproc->name, &child);
    if (err) {
        proc_unlock(curproc);
        return err;
    }

    if ((err = copy_proc(child, curproc))) {
        goto error;
    }

    current_lock();
    thread_lock(child->main_thread);

    if ((err = thread_fork(child->main_thread, current))) {
        thread_unlock(child->main_thread);
        current_unlock();
        goto error;
    }

    current_unlock();

    if ((err = enqueue_global_thread(child->main_thread))) {
        thread_unlock(child->main_thread);
        goto error;
    }

    pid_t child_pid = child->pid;

    if (err) {
        thread_unlock(child->main_thread);
        goto error;
    }

    proc_unlock(curproc);

    err = thread_schedule(child->main_thread);
    thread_unlock(child->main_thread);

    debuglog();

    proc_unlock(child);
    if (err) {
        goto error;
    }

    return child_pid;
error:
    proc_unlock(curproc);
    proc_free(child);
    return err;
}