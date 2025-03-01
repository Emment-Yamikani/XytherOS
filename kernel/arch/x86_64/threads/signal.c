#include <arch/cpu.h>
#include <bits/errno.h>
#include <core/debug.h>
#include <string.h>
#include <sys/schedule.h>
#include <sys/thread.h>

void x86_64_mctx_init(mcontext_t *mctx, bool user, sigaction_t *act, siginfo_t *siginfo, ucontext_t *uctx) {
    memset(mctx, 0, sizeof *mctx);
    if (user) {
        mctx->cs = SEG_UCODE64 << 3;
        mctx->ds = SEG_UDATA64 << 3;
        mctx->fs = SEG_UDATA64 << 3;
        mctx->ss = SEG_UDATA64 << 3;
    } else {
        mctx->cs = SEG_KCODE64 << 3;
        mctx->ds = SEG_KDATA64 << 3;
        mctx->fs = SEG_KDATA64 << 3;
        mctx->ss = SEG_KDATA64 << 3;
    }

    mctx->rip   = (u64)act->sa_handler;
    mctx->rflags= LF_IF;
    mctx->rdi   = (u64)siginfo->si_signo;

    if (act->sa_flags & SA_SIGINFO) {
        mctx->rsi   = (u64)siginfo;
        mctx->rdx   = (u64)uctx;
    }
}

int x86_64_uthread_dispatch(arch_thread_t *arch, sigaction_t *act, siginfo_t *siginfo);

static void x86_64_signal_deliver(arch_thread_t *arch) {
    bool in_sigctx = thread_issigctx(arch->t_thread);

    thread_setsigctx(arch->t_thread);

    sched();

    if (in_sigctx == false) {
        thread_mask_sigctx(arch->t_thread);
    }
}

void x86_64_signal_return(void) {
    debuglog();
    loop();
}

static void x86_64_signal_start(void) {
    debuglog();
    loop();
}


static int x86_64_ksignal_dispatch(arch_thread_t *arch, sigaction_t *act, siginfo_t *siginfo) {
    if (!arch || !arch->t_thread || !act || !act->sa_handler)
        return -EINVAL;

    if (act->sa_flags & SA_ONSTACK) {
        context_t   *ctx    = NULL;
        mcontext_t  *mctx   = NULL;
        uintptr_t   *kstack = NULL;

        // Ensure a valid altsigstack.
        if (!arch->t_altstack.ss_sp || !arch->t_altstack.ss_size)
        return -EINVAL;
        
        // Ignore the altsigstack and execute on the current stack?
        if (arch->t_altstack.ss_flags & SS_DISABLE)
            goto __no_altstack;

        // Indicate that the thread will now be executing on altsigstack.
        arch->t_altstack.ss_flags |= SS_ONSTACK;

        kstack      = arch->t_altstack.ss_sp - sizeof *mctx;
        *--kstack   = (u64)x86_64_signal_return;

        mctx = (mcontext_t *)kstack;
        memset(mctx, 0, sizeof *mctx);

        mctx->ss    = SEG_KDATA64 << 3;
        mctx->rbp   = (u64)kstack;
        mctx->rsp   = (u64)kstack;
        mctx->rflags= (u64)LF_IF;
        mctx->cs    = SEG_KCODE64 << 3;
        mctx->rip   = (u64)act->sa_handler;
        mctx->rdi   = siginfo->si_signo;

        if (act->sa_flags & SA_SIGINFO) {
            mctx->rsi   = (u64)siginfo;
            mctx->rdx   = (u64)arch->t_uctx;
        }

        mctx->ds    =  SEG_KDATA64 << 3;
        mctx->fs    =  SEG_KDATA64 << 3;
        kstack      = (uintptr_t *)mctx;
        *--kstack   = (u64)trapret;
        
        ctx         = (context_t *)((u64)kstack - sizeof *ctx);
        ctx->rip    = (u64)x86_64_signal_start;
        ctx->rbp    = mctx->rsp;
        ctx->link   = arch->t_ctx;

        arch->t_ctx = ctx;

        x86_64_signal_deliver(arch);
        return 0;
    }

__no_altstack:
    current_unlock();
    if (act->sa_flags & SA_SIGINFO) {
        act->sa_handler(siginfo->si_signo, siginfo, arch->t_uctx);
    } else act->sa_handler(siginfo->si_signo);
    current_lock();
    return 0;
}

int x86_64_signal_dispatch(arch_thread_t *arch, sigaction_t *act, siginfo_t *siginfo) {
    // mcontext_t  *mctx   = NULL;
    // ucontext_t  *uctx   = NULL;
    // context_t   *ctx    = NULL;
    // uintptr_t   *kstack = NULL;

    if (!arch || !arch->t_thread || !act || !act->sa_handler)
        return -EINVAL;
    
    if (current_isuser()) {

    } else {
        x86_64_ksignal_dispatch(arch, act, siginfo);
    }

    return 0;
}