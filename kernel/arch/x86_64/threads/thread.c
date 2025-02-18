#include <arch/ucontext.h>
#include <arch/thread.h>
#include <arch/x86_64/asm.h>
#include <arch/x86_64/mmu.h>
#include <bits/errno.h>
#include <core/debug.h>
#include <string.h>
#include <sys/schedule.h>
#include <sys/thread.h>

__noreturn void x86_64_thread_exit(u64 exit_code) {
    current_recursive_lock();
    current_enter_state(T_ZOMBIE);
    current->t_info.ti_exit = exit_code;
    sched();
    loop();
}

/// @brief all threads start here
static void x86_64_thread_start(void) {
    current_unlock();
}

/// @brief all threads end executution here.
static void x86_64_thread_stop(void) {
    u64 rax = rdrax();
    x86_64_thread_exit(rax);
}

/***********************************************************************************
 *                      x86_64 specific thread library functions.                  *
 * *********************************************************************************/
#if defined __x86_64__

int x86_64_kthread_init(arch_thread_t *thread, thread_entry_t entry, void *arg) {
    context_t   *ctx    = NULL;
    mcontext_t  *mctx   = NULL;
    u64         *kstack = NULL;

    if (thread == NULL || thread->t_thread == NULL || entry == NULL)
        return -EINVAL;

    thread_assert_locked(thread->t_thread);

    mctx = (mcontext_t *)ALIGN16(thread->t_sstack.ss_sp - sizeof *mctx);

    memset(mctx, 0, sizeof *mctx);

    kstack      = (u64 *)ALIGN16(thread->t_rsvd);
    *--kstack   = (u64)x86_64_thread_stop;
    
    mctx->ss    = (SEG_KDATA64 << 3);
    mctx->rbp   = (u64)kstack;
    mctx->rsp   = (u64)kstack;
    mctx->rflags= LF_IF;
    mctx->cs    = (SEG_KCODE64 << 3);
    mctx->rip   = (u64)entry;
    mctx->rdi   = (u64)arg;
    mctx->fs    = (SEG_KDATA64 << 3);
    mctx->ds    = (SEG_KDATA64 << 3);

    kstack      = (u64 *)mctx;
    *--kstack   = (u64)trapret;
    ctx         = (context_t *)((u64)kstack - sizeof *ctx);
    ctx->rip    = (u64)x86_64_thread_start;
    ctx->rbp    = mctx->rsp;
    ctx->link   = NULL; // starts with no link to old context.

    thread->t_ctx = ctx;
    return 0;
}

int x86_64_uthread_init(arch_thread_t *thread, thread_entry_t entry, void *arg) {
    int         err     = 0;
    context_t   *ctx    = NULL;
    mcontext_t  *mctx   = NULL;
    u64         *kstack = NULL;
    u64         *ustack = NULL;

    if (thread == NULL || thread->t_thread == NULL || entry == NULL)
        return -EINVAL;

    thread_assert_locked(thread->t_thread);

    if ((ustack = (u64 *)ALIGN16(thread->t_ustack.ss_sp)) == NULL)
        return -EINVAL;

    if ((err = arch_map_n(((u64)ustack) - PGSZ, PGSZ, thread->t_ustack.ss_flags)))
        return err;

    kstack = (u64 *)thread->t_sstack.ss_sp;
    
    assert(kstack, "Invalid Kernel stack.");
    
    assert(is_aligned16(kstack), "Kernel stack is not 16bytes aligned!");

    *--kstack   = (u64)x86_64_thread_stop;
    *--ustack   = MAGIC_RETADDR; // push dummy return address.

    mctx = (mcontext_t *)((u64)kstack - sizeof *mctx);

    memset(mctx, 0, sizeof *mctx);

    mctx->ss      = (SEG_UDATA64 << 3) | DPL_USR;
    mctx->rbp     = mctx->rsp = (u64)ustack;
    mctx->rflags  = LF_IF;
    mctx->cs      = (SEG_UCODE64 << 3) | DPL_USR;
    mctx->rip     = (u64)entry;
    mctx->rdi     = (u64)arg;

    mctx->fs      = (SEG_UDATA64 << 3) | DPL_USR;
    mctx->ds      = (SEG_UDATA64 << 3) | DPL_USR;

    kstack        = (u64 *)mctx;
    *--kstack     = (u64)trapret;
    ctx           = (context_t *)((u64)kstack - sizeof *ctx);
    ctx->rip      = (u64)x86_64_thread_start;
    ctx->rbp      = mctx->rsp;
    ctx->link     = NULL; // starts with no link to old context.

    thread->t_ctx   = ctx;
    return 0;
}

int x86_64_thread_execve(arch_thread_t *thread, thread_entry_t entry, int argc, const char *argp[], const char *envp[]) {
    int         err         = 0;
    context_t   *ctx        = NULL;
    mcontext_t  *mctx       = NULL;
    u64         *ustack     = NULL;
    u64         *kstack     = NULL;

    if (thread == NULL || thread->t_thread == NULL || entry == NULL)
        return -EINVAL;

    thread_assert_locked(thread->t_thread);

    if ((ustack = (u64 *)ALIGN16(thread->t_ustack.ss_sp)) == NULL)
        return -EINVAL;

    if ((err = arch_map_n(((u64)ustack) - PGSZ, PGSZ, thread->t_ustack.ss_flags)))
        return err;

    kstack = (u64 *)thread->t_sstack.ss_sp;
    
    assert(kstack, "Invalid Kernel stack.");
    
    assert(is_aligned16(kstack), "Kernel stack is not 16bytes aligned!");

    *--kstack = (u64)x86_64_thread_stop;
    *--ustack = MAGIC_RETADDR; // push dummy return address.

    mctx = (mcontext_t *)((u64)kstack - sizeof *mctx);

    memset(mctx, 0, sizeof *mctx);

    mctx->ss      = (SEG_UDATA64 << 3) | DPL_USR;
    mctx->rbp     = mctx->rsp = (u64)ustack;
    mctx->rflags  = LF_IF;
    mctx->cs      = (SEG_UCODE64 << 3) | DPL_USR;
    mctx->rip     = (u64)entry;

    // pass paramenters to entry function.
    mctx->rdi     = (u64)argc;
    mctx->rsi     = (u64)argp;
    mctx->rdx     = (u64)envp;

    mctx->fs      = (SEG_UDATA64 << 3) | DPL_USR;
    mctx->ds      = (SEG_UDATA64 << 3) | DPL_USR;

    kstack        = (u64 *)mctx;
    *--kstack     = (u64)trapret;
    ctx           = (context_t *)((u64)kstack - sizeof *ctx);
    ctx->rip      = (u64)x86_64_thread_start;
    ctx->rbp      = mctx->rsp;
    ctx->link     = NULL; // starts with no link to old context.
    
    thread->t_ctx = ctx;
    return 0;
}

int x86_64_thread_setkstack(arch_thread_t *thread) {
    u64     kstack  = 0;

    if (thread == NULL || thread->t_thread == NULL)
        return -EINVAL;

    thread_assert_locked(thread->t_thread);

    if (thread->t_rsvd < thread->t_kstack.ss_sp)
        return -EOVERFLOW;

    kstack = (u64)thread->t_rsvd;
    tss_set(kstack, (SEG_KDATA64 << 3));
    return 0;
}

int x86_64_thread_fork(arch_thread_t *dst, arch_thread_t *src) {
    mcontext_t  *mctx   = NULL;
    context_t   *ctx    = NULL;
    u64         *kstack = NULL;

    if (dst == NULL || dst->t_thread == NULL || src == NULL || src->t_thread)
        return -EINVAL;

    thread_assert_locked(dst->t_thread);
    thread_assert_locked(src->t_thread);

    kstack = (u64 *)dst->t_sstack.ss_sp;
    
    assert(kstack, "Invalid Kernel stack.");
    
    assert(is_aligned16(kstack), "Kernel stack is not 16bytes aligned!");

    *--kstack = (u64)x86_64_thread_stop;

    mctx        = (mcontext_t *)(((u64)kstack) - sizeof *mctx);
    *mctx       = src->t_uctx->uc_mcontext;
    mctx->rax   = 0;

    kstack      = (u64 *)mctx;
    *--kstack   = (u64)trapret;
    ctx         = (context_t *)(((u64)kstack) - sizeof *ctx);
    ctx->rip    = (u64)x86_64_thread_start;
    ctx->rbp    = (u64)dst->t_rsvd;
    ctx->link   = NULL;
    dst->t_ctx  = ctx;
    return 0;
}

void x86_64_thread_free(arch_thread_t *arch) {
    if (arch == NULL)
        return;
    

}

#endif // __x86_64__