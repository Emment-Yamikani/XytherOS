#include <bits/errno.h>
#include <fs/fs.h>
#include <string.h>
#include <mm/kalloc.h>
#include <sys/proc.h>
#include <sys/elf/elf.h>
#include <sys/binary_loader.h>

proc_t *initproc = NULL;
queue_t *procQ   = QUEUE_NEW();

// bucket to hold free'd PIDs.
static queue_t *procIDs = QUEUE_NEW();

/**
 * For process ID alocation.
 * to be moved to sys/proc.c
 */
static atomic_t pids = {0};

static int proc_alloc_pid(pid_t *ref) {
    int     err = 0;
    pid_t   pid = 0;

    if (ref == NULL)
        return -EINVAL;
    
    queue_lock(procIDs);
    err = dequeue(procIDs, (void **)&pid);
    queue_unlock(procIDs);

    if (err == 0) goto done;

    pid = (pid_t) atomic_inc_fetch(&pids);
    if (pid >= NPROC)
        return -EAGAIN;
done:
    *ref = pid;
    return 0;
}

static void proc_free_pid(pid_t pid) {
    if (pid <= 0 || pid > NPROC)
        return;
}

int procQ_remove(proc_t *proc) {
    int err = 0;

    if (proc == NULL)
        return -EINVAL;
    
    proc_assert_locked(proc);

    queue_lock(procQ);
    if ((err = embedded_queue_remove(procQ, &proc->proc_qnode)) == 0) {
        proc_putref(proc);
    }
    queue_unlock(procQ);

    return err;
}

int procQ_insert(proc_t *proc) {
    int err = 0;
    
    if (proc == NULL)
        return -EINVAL;
    
    queue_lock(procQ);
    if (0 == (err = embedded_enqueue(procQ, &proc->proc_qnode, QUEUE_UNIQUE))) {
        proc_getref(proc);
    }
    queue_unlock(procQ);

    return err;
}

int procQ_search_bypid(pid_t pid, proc_t **ref) {
    queue_lock(procQ);
    
    proc_t *proc;
    foreach_process(procQ, proc) {
        proc_lock(proc);
        if (proc->pid == pid) {
            if (ref != NULL) {
                *ref = proc_getref(proc);
            } else {
                proc_unlock(proc);
            }

            queue_lock(procQ);
            return 0;
        }
        proc_unlock(proc);
    }

    queue_lock(procQ);

    return -ESRCH;
}

int procQ_search_bypgid(pid_t pgid, proc_t **ref) {
    queue_lock(procQ);
    
    proc_t *proc;
    foreach_process(procQ, proc) {
        proc_lock(proc);
        if (proc->pgid == pgid) {
            if (ref != NULL) {
                *ref = proc_getref(proc);
            } else {
                proc_unlock(proc);
            }

            queue_lock(procQ);
            return 0;
        }
        proc_unlock(proc);
    }

    queue_lock(procQ);

    return -ESRCH;
}

int proc_alloc(const char *name, proc_t **pref) {
    int         err     = 0;
    proc_t     *proc    = NULL;
    mmap_t     *mmap    = NULL;
    thread_t   *thread  = NULL;

    if (name == NULL || pref == NULL) {
        return -EINVAL;
    }

    if (NULL == (proc = kzalloc(sizeof *proc))) {
        return -ENOMEM;
    }

    if ((err = mmap_alloc(&mmap))) {
        goto error;
    }

    if ((err = thread_alloc(KSTACK_SIZE, THREAD_CREATE_USER, &thread))) {
        goto error;
    }

    if ((err = thread_create_group(thread))) {
        goto error;
    }

    err = -ENOMEM;
    if (NULL == (proc->name = strdup(name))) {
        goto error;
    }

    if ((err = proc_alloc_pid(&proc->pid))) {
        goto error;
    }

    proc->refcnt        = 1;
    proc->mmap          = mmap;
    proc->pgid          = proc->pid;
    proc->sid           = proc->pid;
    proc->child_event   = COND_INIT();
    proc->lock          = SPINLOCK_INIT();
    proc->cred          = thread->t_cred;
    proc->fctx          = thread->t_fctx;
    proc->threads       = thread->t_group;
    proc->signals       = thread->t_signals;

    proc->main_thread   = thread;

    thread->t_mmap      = mmap;
    proc_lock(proc);

    thread->t_proc      = proc_getref(proc);
    mmap_unlock(mmap);

    thread_unlock(thread);

    *pref = proc;
    return 0;
error:
    if (proc->name) {
        kfree(proc->name);
    }

    if (thread) {
        thread_free(thread);
    }

    if (mmap) {
        mmap_free(mmap);
    }

    if (proc) {
        kfree(proc);
    }
    return err;
}

void proc_free(proc_t *proc) {
    if (proc == NULL) {
        return;
    }
    
    if (curproc == proc) {
        panic("ERROR: process cannot destroy self\n");
    }
    
    if (!proc_islocked(proc)) {
        proc_lock(proc);
    }

    proc->refcnt--;

    /**
     * @brief If the refcnt of proc is zero(0),
     * release all resources allocated to it.
     * However, if at all we have some thread(s) sleeping(wating) for
     * a broadcast on 'proc->wait' to be fired, desaster may unfold.
     */
    if (proc->refcnt <= 0) {
        queue_lock(&proc->children);
        queue_flush(&proc->children);
        queue_unlock(&proc->children);

        if (proc_mmap(proc)) {
            mmap_free(proc_mmap(proc));
        }

        if (proc->name) {
            kfree(proc->name);
        }

        proc_free_pid(proc->pid);

        proc_unlock(proc);

        /// TODO: a solution to the problem above
        /// may be to kill the thread(s) waiting
        /// to avoid access to resources that have already been released.
        kfree(proc);
        return;
    }

    proc_unlock(proc);
}

int proc_load(const char *pathname, mmap_t *mmap, thread_entry_t *entry) {
    int         err     = 0;
    uintptr_t   pdbr    = 0;
    inode_t     *binary = NULL;
    dentry_t    *dentry = NULL;

    if (mmap == NULL || entry == NULL) {
        return -EINVAL;
    }

    if ((err = vfs_lookup(pathname, NULL,  O_EXEC, &dentry))) {
        goto error;
    }

    binary = dentry->d_inode;

    err = -ENOENT;
    if (binary == NULL) {
        goto error;
    }

    /// check file  type and only execute an appropriate file type.
    ilock(binary);
    if (IISDEV(binary)) {
        err = -ENOEXEC;
        iunlock(binary);
        goto error;
    } else if (IISDIR(binary)) {
        err = -EISDIR;
        iunlock(binary);
        goto error;
    } else if (IISFIFO(binary)) {
        err = -ENOEXEC;
        iunlock(binary);
        goto error;
    } else if (IISSYM(binary)) {
        err = -ENOEXEC;
        iunlock(binary);
        goto error; 
    }

    foreach_binary_loader() {
        /// check the binary image to make sure it is a valid program file.
        if ((err = loader->check(binary))) {
            iunlock(binary);
            goto error;
        }
    
        /// load the binary image into memory in readiness for execution.
        if ((err = loader->load(binary, mmap, entry)) == 0) {
            goto commit;
        }
    }

    /// binary file not loaded ???.
    iunlock(binary);
    goto error;
commit:
    iunlock(binary);
    /**
     * @brief close this dentry.
     * dentry->d_inode must have been opened.
     * To remain persistent.
     */
    dclose(dentry);
    return 0;
error:
    if (pdbr) {
        arch_switch_pgdir(pdbr, NULL);
    }

    if (dentry) {
        dclose(dentry);
    }

    return err;
}

int proc_init(const char *initpath) {
    int         err     = 0;
    uintptr_t   pdbr    = 0;
    proc_t      *proc   = NULL;
    const char  *envp[] = { NULL, NULL };
    const char  *argp[] = { initpath, NULL };

    if ((err = proc_alloc(initpath, &proc))) {
        goto error;
    }

    proc_mmap_lock(proc);
    if ((err = mmap_set_focus(proc_mmap(proc), &pdbr))) {
        proc_mmap_unlock(proc);
        goto error;
    }

    if ((err = proc_load(initpath, proc_mmap(proc), &proc->entry))) {
        proc_mmap_unlock(proc);
        goto error;
    }

    thread_lock(proc->main_thread);
    
    proc->main_thread->t_info.ti_entry = proc->entry;

    if ((err = thread_execve(proc->main_thread, argp, envp))) {
        thread_unlock(proc->main_thread);
        goto error;
    }

    proc_mmap_unlock(proc);

    if ((err = thread_schedule(proc->main_thread))) {
        thread_unlock(proc->main_thread);
        goto error;
    }

    thread_unlock(proc->main_thread);

    if ((err = procQ_insert(proc))) {
        proc_unlock(proc);
        goto error;
    }

    initproc = proc;
    proc_unlock(proc);

    return 0;
error:
    if (proc) {
        proc_free(proc);
    }

    return err;
}

/********************************************************************************/
/***********************    PROCESS QUEUE HELPERS    ****************************/
/********************************************************************************/

int proc_add_child(proc_t *parent, proc_t *child) {
    int err = 0;

    if (parent == NULL || child == NULL || child->parent) {
        return -EINVAL;
    }
    
    proc_assert_locked(child);
    proc_assert_locked(parent);

    queue_lock(&parent->children);
    if ((err = embedded_enqueue(&parent->children, &child->child_qnode, QUEUE_UNIQUE)) == 0) {
        child->parent = parent;
        proc_getref(child);
    }
    queue_unlock(&parent->children);

    return err;
}

int proc_remove_child(proc_t *parent, proc_t *child) {
     int err = 0;

    if (parent == NULL || child == NULL) {
        return -EINVAL;
    }

    if (child->parent != parent) {
        return -EINVAL;
    }
    
    proc_assert_locked(child);
    proc_assert_locked(parent);

    queue_lock(&parent->children);
    if ((err = embedded_queue_remove(&parent->children, &child->child_qnode)) == 0) {
        proc_putref(child);
        child->parent = NULL;
    }
    queue_unlock(&parent->children);

    return err;
}

int proc_abandon_children(proc_t *new_parent, proc_t *old_parent) {
    if (new_parent == NULL || old_parent == NULL) {
        return -EINVAL;
    }

    proc_assert_locked(old_parent);
    proc_assert_locked(new_parent);

    queue_t *srcQ = &old_parent->children;
    queue_t *dstQ = &new_parent->children;
    
    queue_lock(dstQ);
    queue_lock(srcQ);
    
    proc_t *child;
    queue_foreach_entry(srcQ, child, child_qnode) {
        proc_lock(child);
        int err = 0;
        queue_node_t *node = &child->child_qnode;
        if ((err = embedded_queue_relloc(dstQ, srcQ, node, QUEUE_UNIQUE, QUEUE_TAIL))) {
            proc_unlock(child);
            break;
        }

        child->parent = proc_getref(new_parent);
        proc_putref(old_parent);
        proc_unlock(child);
    }

    queue_unlock(srcQ);
    queue_unlock(dstQ);
    return 0;
}