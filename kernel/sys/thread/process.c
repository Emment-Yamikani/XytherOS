#include <bits/errno.h>
#include <core/debug.h>
#include <mm/kalloc.h>
#include <string.h>
#include <sys/thread.h>

pid_t alloc_pid(void) {
    static _Atomic(pid_t) pid = 0;
    return atomic_inc_fetch(&pid);
}

void proc_free(proc_t *proc) {
    assert(proc, "Invalid proc.\n");

    spin_recursive_lock(&proc->lock);

    if (proc->name) kfree(proc->name);
    
    spin_lock(&proc->lock);
    
    kfree(proc);
}

static int proc_init(proc_t *proc, const char *name) {
    int err;

    if (proc == NULL || name == NULL)
        return -EINVAL;
    
    memset(proc, 0, sizeof *proc);

    if (NULL == (proc->name = strdup(name)))
        return -ENOMEM;

    if ((err = cond_init(&proc->child_event)))
        goto error;

    if ((err = queue_init(&proc->children)))
        goto error;

    proc->entry = NULL;
    proc->parent= NULL;
    proc->state = P_EMBRYO;

    spinlock_init(&proc->lock);

    return 0;
error:
    if (proc->name) kfree(proc->name);
    return err;
}

static int proc_new(thread_t **ptp) {
    int err;
    thread_t *thread = NULL;

    if (ptp == NULL)
        return -EINVAL;

    if ((err = thread_alloc(KSTACK_SIZE, THREAD_USER, &thread)))
        return err;

    if ((err = mmap_alloc(&thread->t_mmap))) {
        thread_free(thread);
        return err;
    }

    mmap_unlock(thread->t_mmap);

    if ((err = thread_create_group(thread))) {
        mmap_free(thread->t_mmap);
        thread_free(thread);
        return err;
    }

    *ptp = thread;
    return 0;
}

int proc_alloc(const char *name, thread_t **ptp) {
    int         err;
    thread_t    *thread = NULL;

    if (name == NULL || ptp == NULL)
        return -EINVAL;

    if ((err = proc_new(&thread)))
        return err;

    if (NULL == (thread->t_proc = (proc_t *)kmalloc(sizeof(proc_t))))
        return -EINVAL;

    if ((err = proc_init(thread->t_proc, name)))
        goto error;

    *ptp = thread;

    return 0;
error:
    if (thread->t_proc) proc_free(thread->t_proc);

    if (thread->t_mmap) mmap_free(thread->t_mmap);

    thread_free(thread);

    debug("Err[%s]: Failed to allocate a process for [%s].\n", strerror(err), name);
    return err;
}

int proc_spawn_init(const char *path) {
    int         err;
    uintptr_t   old_pdbr= 0;
    thread_t    *thread = NULL;
    const char  *envp[] = {NULL};
    const char  *argp[] = {path, NULL}; 

    if (path == NULL)   
        return -EINVAL;

    if ((err = proc_alloc(path, &thread)))
        return err;

    mmap_lock(thread->t_mmap);

    if ((err = mmap_focus(thread->t_mmap, &old_pdbr)))
        goto error;

    todo("Load ELF for %s\n", path);

    if ((err = thread_execve(thread, argp, envp)))
        goto error;

    arch_swtchvm(old_pdbr, NULL);
    mmap_unlock(thread->t_mmap);

    if ((err = thread_schedule(thread)))
        goto error;

    thread_unlock(thread);
    return 0;
error:

    if (old_pdbr)
        arch_swtchvm(old_pdbr, NULL);

    proc_free(thread->t_proc);
    mmap_free(thread->t_mmap);
    thread_free(thread);

    debug("Error[%s]: Failed to spawn [%s]\n", strerror(err), path);
    return err;
}