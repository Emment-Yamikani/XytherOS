#include <arch/cpu.h>
#include <bits/errno.h>
#include <core/debug.h>
#include <string.h>
#include <sys/schedule.h>
#include <sys/thread.h>

const ulong ARCH_NSIG_NESTED = 8;

static void x86_64_signal_start(void) {
    /*
     * `current->t_arch.t_context' is the dispatch (signal) context created during signal delivery.
     * Its link field must have been set to the scheduler context (sched_ctx)
     * that was current before the signal arrived.
     *
     * To allow the signal handler to be preemptible, we set the current context
     * to the scheduler context.
     */
    if (!current->t_arch.t_context || !current->t_arch.t_context->link) {
        panic("Invalid dispatch context in signal_start");
    }
    context_t *sched_ctx = current->t_arch.t_context->link;
    sched_ctx->link = current->t_arch.t_context;

    // We expect sched_ctx->link to still be intact (pointing back to `current->t_arch.t_context')
    // so that later we can restore it.
    current->t_arch.t_context = sched_ctx; 
    current->t_arch.t_altstack.ss_flags |= SS_ONSTACK;
    current_unlock();
}

void x86_64_signal_return(void) {
    /*
     * When returning from the signal handler, the current context is the
     * scheduler context (sched_ctx) that was set by x86_64_signal_start.
     * We then want to switch to the dispatch context (`current->t_arch.t_context') so that
     * the signal delivery code (signal_dispatch) can complete and, upon return,
     * the original scheduler context is restored.
     */
    context_t *sched_ctx = current->t_arch.t_context;
    if (!sched_ctx || !sched_ctx->link) {
        panic("Invalid context chain in signal_return");
    }

    current->t_arch.t_context = sched_ctx->link;
    current->t_arch.t_context->link = sched_ctx;
    current_lock();
    sched();
}

static void x86_64_signal_onstack(void) {
    bool is_sigctx = current_issigctx();

    current_setsigctx();

    sched();
    current->t_arch.t_altstack.ss_flags &= ~SS_ONSTACK;

    if (is_sigctx == false)
        current_mask_sigctx();
}

static int x86_64_signal_bycall(arch_thread_t *arch, sigaction_t *act, siginfo_t *siginfo) {
    bool is_sigctx = current_issigctx();

    current_setsigctx();

    current_unlock();

    sti();

    if (act->sa_flags & SA_SIGINFO)
        act->sa_handler(siginfo->si_signo, siginfo, arch->t_uctx);
    else act->sa_handler(siginfo->si_signo);

    cli();

    current_lock();

    if (is_sigctx == false)
        current_mask_sigctx();
    return 0;
}

int x86_64_signal_dispatch(arch_thread_t *arch, sigaction_t *act, siginfo_t *siginfo) {
    if (!arch || !arch->t_thread || !act || !act->sa_handler || !siginfo)
        return -EINVAL;

    if (act->sa_flags & SA_ONSTACK) {
        uintptr_t   *kstack     = NULL;
        context_t   *context    = NULL;
        mcontext_t  *mcontext   = NULL;

        if (!arch->t_altstack.ss_sp || !arch->t_altstack.ss_size) {
            return -EINVAL;
        }

        if (arch->t_altstack.ss_flags & SS_DISABLE) {
            return x86_64_signal_bycall(arch, act, siginfo);
        }

        if (arch->t_altstack.ss_flags & SS_ONSTACK) {
            return x86_64_signal_bycall(arch, act, siginfo);
        }

        mcontext = (mcontext_t *)ALIGN16(arch->t_sstack.ss_sp - sizeof *mcontext);
        memset(mcontext, 0, sizeof *mcontext);

        kstack    = (uintptr_t *)ALIGN16(arch->t_altstack.ss_sp);
        *--kstack = (uintptr_t)x86_64_signal_return;

        mcontext->ss    = SEG_KDATA64 << 3;
        mcontext->rsp   = (u64)kstack;
        mcontext->rbp   = (u64)kstack;
        mcontext->rflags= LF_IF;
        mcontext->cs    = SEG_KCODE64 << 3;
        mcontext->rip   = (u64)act->sa_handler;
        mcontext->rdi   = siginfo->si_signo;
        mcontext->ds    = SEG_KDATA64 << 3;
        mcontext->fs    = SEG_KDATA64 << 3;

        if (act->sa_flags & SA_SIGINFO) {
            mcontext->rsi = (u64)siginfo;
            mcontext->rdx = (u64)arch->t_uctx;
        }

        kstack    = (uintptr_t *)mcontext;
        *--kstack = (uintptr_t)trapret;

        context         = (context_t *)((uintptr_t)kstack - sizeof *context);
        memset(context, 0, sizeof *context);

        context->rip    = (u64)x86_64_signal_start;
        context->rbp    = (u64)mcontext->rbp;
        context->link   = arch->t_context;
        arch->t_context = context;

        x86_64_signal_onstack();

        arch->t_context = arch->t_context->link;
        return 0;
    }

    return x86_64_signal_bycall(arch, act, siginfo);
}