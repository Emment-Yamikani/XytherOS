#include <arch/paging.h>
#include <bits/errno.h>
#include <core/debug.h>
#include <mm/kalloc.h>
#include <string.h>
#include <sys/thread.h>

/**
 * @brief Allocate a new thread ID.
 *
 * Uses a static atomic counter to generate unique thread IDs.
 *
 * @return A unique thread ID.
 */
static tid_t alloc_tid(void) {
    static atomic_t tid = 0;
    return atomic_inc_fetch(&tid);
}

/**
 * @brief Allocate a kernel stack for a thread.
 *
 * This function allocates a kernel stack of size KSTACK_SIZE.
 *
 * @param[in]  kstack_size Size of kernel stack.
 * @param[out] pp Pointer to the allocated stack address.
 * @return 0 on success, or -EINVAL/-error code on failure.
 */
static int thread_alloc_kstack(usize kstack_size, void **pp) {
    int err;
    uintptr_t addr;
    if (pp == NULL || ((kstack_size < KSTACK_SIZE) || (kstack_size > KSTACK_MAXSIZE)))
        return -EINVAL;
    if ((err = arch_pagealloc(kstack_size, &addr)))
        return err;
    *pp = (void *)addr;
    return 0;
}

/**
 * @brief Allocate and initialize a new thread structure.
 *
 * This function allocates a kernel stack, initializes the thread
 * structure at the top of the stack, and sets up architecture-specific
 * data, scheduling, and synchronization fields.
 *
 * @param kstack_size Size of kernel stack.
 * @param flags Creation flags (e.g., THREAD_CREATE_USER, THREAD_CREATE_DETACHED).
 * @param[out] ptp Pointer to the allocated thread structure.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int thread_alloc(usize kstack_size, int flags, thread_t **ptp) {
    int             err     = 0;
    uintptr_t       stack   = 0;
    arch_thread_t   *arch   = NULL;
    thread_sched_t  *sched  = NULL;
    thread_info_t   *tinfo  = NULL;
    thread_t        *thread = NULL;

    if (ptp == NULL)
        return -EINVAL;

    if ((err = thread_alloc_kstack(kstack_size, (void **)&stack)))
        return err;

    /* Place the thread structure at the top of the allocated stack */
    thread = (thread_t *)ALIGN16((stack + kstack_size) - sizeof(*thread));

    memset(thread, 0, sizeof(*thread));

    /* Initialize architecture-specific thread context */
    arch = &thread->t_arch;
    arch->t_thread          = thread;
    arch->t_kstack.ss_size  = kstack_size;
    arch->t_kstack.ss_sp    = (void *)stack;
    arch->t_sstack.ss_sp    = (void *)thread;
    /**
     * 1KiB should be large enough to act as a scratch space for executing
     * the thread for the first time. */
    arch->t_sstack.ss_size  = (usize)KiB(1);
    arch->t_rsvd            = (void *)thread - KiB(1);

    spinlock_init(&thread->t_lock);

    /* Initialize thread information */
    tinfo           = &thread->t_info;
    tinfo->ti_tid   = alloc_tid();
    /* Combine flags for user and detached threads */
    tinfo->ti_flags = ((flags & THREAD_CREATE_USER) ? THREAD_USER : 0) |
                      ((flags & THREAD_CREATE_DETACHED) ? THREAD_DETACHED : 0);

    /* Initialize scheduling information */
    sched = &tinfo->ti_sched;
    sched->ts_ctime         = epoch_get();
    sched->ts_affin.cpu_set = -1; /* -1 means all CPUs allowed */
    sched->ts_affin.type    = SOFT_AFFINITY;

    /* Initialize thread qnodes */
    thread->t_wait_qnode.data   = (void *)thread;
    thread->t_run_qnode.data    = (void *)thread;
    thread->t_group_qnode.data  = (void *)thread;
    thread->t_global_qnode.data = (void *)thread;

    /* Initialize signal queues */
    for (usize i = 0; i < NELEM(thread->t_sigqueue); ++i) {
        queue_init(&thread->t_sigqueue[i]);
    }

    /* Set initial state to embryonic */
    thread_enter_state(thread, T_EMBRYO);

    thread_lock(thread);
    
    *ptp = thread;
    return 0;
}

/**
 * @brief Create a new thread.
 *
 * This function creates either a user thread or a kernel thread depending on
 * the provided flags. When the flag THREAD_CREATE_USER is set the thread is
 * created as a user thread (which requires curproc to be non-NULL); otherwise
 * a kernel thread is created.
 *
 * The provided attribute structure determines per-thread parameters. When
 * attr is NULL, reasonable defaults are chosen:
 *   - For user threads:  detachstate = 0, stackaddr = 0, guardsz = PGSZ,   stacksz = USTACKSZ
 *   - For kernel threads: detachstate = 0, stackaddr = 0, guardsz = PAGESZ,   stacksz = KSTACK_SIZE
 *
 * @param[in]  attr   Pointer to a thread_attr_t structure (or NULL for defaults).
 * @param[in]  entry  Entry point function for the thread.
 * @param[in]  arg    Argument passed to the thread entry point.
 * @param[in]  cflags Creation flags. Use THREAD_CREATE_USER for user threads and
 *                    additional flags (e.g. THREAD_CREATE_DETACHED, THREAD_CREATE_GROUP,
 *                    THREAD_CREATE_SCHED) as appropriate.
 * @param[out] ptp    Pointer to where the created thread_t pointer is stored.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int thread_create(thread_attr_t *attr, thread_entry_t entry, void *arg, int cflags, thread_t **ptp) {
    int             err     = 0;
    thread_attr_t   t_attr  = {0};
    thread_t        *thread = NULL;

    /* Set default attributes if none provided */
    if (attr)
        t_attr = *attr;
    else {
        if (cflags & THREAD_CREATE_USER)
            t_attr = UTHREAD_ATTR_DEFAULT;
        else
            t_attr = KTHREAD_ATTR_DEFAULT;
    }

    cflags |= current == NULL  ? THREAD_CREATE_GROUP : 0;

    // create a self detatching thread.
    cflags |= t_attr.detachstate ? THREAD_CREATE_DETACHED : 0;

    // user requested a USER thread creation?
    if (cflags & THREAD_CREATE_USER) {
        uc_stack_t  uc_stack = {0};
        vmr_t       *ustack  = NULL;

        if (!current || !current_isuser())
            return -EINVAL;

        if (cflags & THREAD_CREATE_GROUP)
            return -EINVAL;

        if ((err = thread_alloc(KSTACK_SIZE, cflags, &thread)))
            return err;

        mmap_lock(current->t_mmap);

        if (t_attr.stackaddr == 0) {
            if ((err = mmap_alloc_stack(current->t_mmap, t_attr.stacksz, &ustack))) {
                mmap_unlock(current->t_mmap);
                goto error;
            }
        } else {
            err = -EINVAL;
            if (NULL == (ustack = mmap_find(current->t_mmap, t_attr.stackaddr))) {
                mmap_unlock(current->t_mmap);
                goto error;
            }

            if (__isstack(ustack) == 0) {
                mmap_unlock(current->t_mmap);
                goto error;
            }
        }

        uc_stack.ss_size    = __vmr_size(ustack);
        uc_stack.ss_flags   = __vmr_vflags(ustack);
        uc_stack.ss_sp      = (void *)__vmr_upper_bound(ustack);

        mmap_unlock(current->t_mmap);

        /// TODO: Optmize size of ustack according to attr;
        /// TODO: maybe perform a split? But then this will mean
        /// free() and unmap() calls to reverse malloc() and mmap() respectively
        /// for this region may fail. ???
        thread->t_arch.t_ustack = uc_stack;

        thread->t_proc = current->t_proc;

        if ((err = arch_uthread_init(&thread->t_arch, entry, arg)))
            goto error;

        // add thread to current thread group.
        if ((err = thread_join_group(thread))) {
            goto error;
        }
    } else { // create a kernel thread instead.
        // allocate the thread struct and kernel struct.
        if ((err = thread_alloc(t_attr.stacksz, cflags, &thread)))
            return err;

        // Initialize the kernel execution context.
        if ((err = arch_kthread_init(&thread->t_arch, entry, arg)))
            goto error;

        // Do we want to create a new thread group?
        if (cflags & THREAD_CREATE_GROUP) {
            // If so create a thread group and make thread the main thread.
            if ((err = thread_create_group(thread)))
                goto error;
        } else {
            // add thread to current thread group.
            if ((err = thread_join_group(thread))) {
                goto error;
            }
        }
    }

    // set the thread's entry point.
    thread->t_info.ti_entry  = entry;

    // Insert this new thread in the global thread queue.
    queue_lock(global_thread_queue);
    if ((err = embedded_enqueue(global_thread_queue, &thread->t_global_qnode, QUEUE_ENFORCE_UNIQUE))) {
        queue_unlock(global_thread_queue);
        goto error;
    }

    queue_unlock(global_thread_queue);

    // schedule the newly created thread?
    if (cflags & THREAD_CREATE_SCHED) {
        if ((err = thread_schedule(thread)))
            goto error;
    }

    if (ptp) *ptp = thread;
    else thread_unlock(thread);

    return 0;
error:
    debug("%s:%d: Failed to create thread, err: %d\n", __FILE__, __LINE__, err);
    if (thread) thread_free(thread);
    return err;
}

void thread_free(thread_t *thread) {
    int             err     = 0;
    arch_thread_t   *arch   = NULL;
    queue_t         *queue  = NULL;

    if (thread == NULL)
        return;
    
    thread_recursive_lock(thread);

    // Detach thread from all queues
    queue_lock(global_thread_queue);
    embedded_queue_remove(global_thread_queue, &thread->t_global_qnode);
    queue_unlock(global_thread_queue);

    if (thread->t_group) {
        queue_lock(thread->t_group);
        embedded_queue_remove(queue, &thread->t_group_qnode);
        queue_unlock(thread->t_group);
    }

    if (thread->t_run_queue) {
        queue_lock(thread->t_run_queue);
        embedded_queue_remove(queue, &thread->t_run_qnode);
        queue_unlock(thread->t_run_queue);
    }

    if (thread->t_wait_queue) {
        queue = thread->t_wait_queue;
        queue_lock(queue);
        embedded_queue_remove(queue, &thread->t_wait_qnode);
        queue_unlock(queue);
    }

    arch = &thread->t_arch;

    arch_thread_free(arch);

    if (thread_isuser(thread)) {
        mmap_lock(thread->t_mmap);
        assert_eq(err = mmap_unmap(thread->t_mmap,
            (uintptr_t)(arch->t_ustack.ss_sp - arch->t_ustack.ss_size),
            arch->t_ustack.ss_size), 0, "Error[%s]: Unmapping vmr.\n", perror(err)
        );
        mmap_unlock(thread->t_mmap);
    }

    for (usize i = 0; i < NELEM(thread->t_sigqueue); ++i) {
        queue_lock(&thread->t_sigqueue[i]);
        queue_flush(&thread->t_sigqueue[i]);
        queue_unlock(&thread->t_sigqueue[i]);
    }

    thread_enter_state(thread, T_TERMINATED);

    thread_unlock(thread);
    arch_pagefree((uintptr_t)arch->t_kstack.ss_sp, arch->t_kstack.ss_size);
}