#include <bits/errno.h>
#include <sys/proc.h>
#include <sys/thread.h>

int thread_fork(thread_t *dst_thread, thread_t *src_thread) {
    if (!dst_thread || !src_thread) {
        return -EINVAL;
    }

    thread_assert_locked(src_thread);
    thread_assert_locked(dst_thread);

    mcontext_t *mctx = &src_thread->t_arch.t_uctx->uc_mcontext;
    uintptr_t sp = mctx->rsp;

    dst_thread->t_info.ti_sched = (thread_sched_t) {
        .ts_priority        = src_thread->t_info.ti_sched.ts_priority,
        .ts_proc        = src_thread->t_info.ti_sched.ts_proc,
        .ts_timeslice   = src_thread->t_info.ti_sched.ts_timeslice,
        .ts_affin.type  = src_thread->t_info.ti_sched.ts_affin.type,
    };

    vmr_t *ustack;
    mmap_lock(dst_thread->t_mmap);
    if ((ustack = mmap_find(dst_thread->t_mmap, sp)) == NULL) {
        mmap_unlock(dst_thread->t_mmap);
        return -EADDRNOTAVAIL;
    }
    mmap_unlock(dst_thread->t_mmap);

    uc_stack_t uc_stack;
    memset(&uc_stack, 0, sizeof uc_stack);

    uc_stack.ss_size     = __vmr_size(ustack);
    uc_stack.ss_flags    = __vmr_vflags(ustack);
    uc_stack.ss_sp       = (void *)__vmr_upper_bound(ustack);
    dst_thread->t_arch.t_ustack = uc_stack;
    return arch_thread_fork(&dst_thread->t_arch, &src_thread->t_arch);
}