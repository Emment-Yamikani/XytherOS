#include <arch/traps.h>
#include <arch/ucontext.h>
#include <arch/x86_64/lapic.h>
#include <core/debug.h>
#include <dev/ps2.h>
#include <dev/timer.h>
#include <sys/schedule.h>
#include <sys/thread.h>
#include <sync/preempt.h>

void dump_tf(mcontext_t *mctx, int halt) {
    void *stack_sp = NULL;
    usize stack_sz = 0;

    if (current) {
        stack_sp   = current->t_arch.t_kstack.ss_sp;
        stack_sz    = current->t_arch.t_kstack.ss_size;
    }

    if (halt) {
        panic("\n\e[00;014mTRAP:%d\e[0m MCTX: %p CPU%d TID[%d:%d] to_ret->%p\n"
              "\e[00;015merr\e[0m=\e[00;012m%16p\e[0m rfl=\e[00;12m%16p\e[0m cs =\e[00;12m%16p\e[0m\n"
              "\e[00;015mds\e[0m =%16p \e[00;015mfs \e[0m=%16p \e[00;015mss \e[0m=%16p\n"
              "\e[00;015mrax\e[0m=\e[00;012m%16p\e[0m rbx=\e[00;12m%16p\e[0m rcx=\e[00;12m%16p\e[0m\n"
              "\e[00;015mrdx\e[0m=%16p \e[00;015mrdi\e[0m=%16p \e[00;015mrsi\e[0m=%16p\n"
              "\e[00;015mrbp\e[0m=\e[00;012m%16p\e[0m rsp=\e[00;12m%16p\e[0m r8 =\e[00;12m%16p\e[0m\n"
              "\e[00;015mr9\e[0m =%16p \e[00;015mr10\e[0m=%16p \e[00;015mr11\e[0m=%16p\n"
              "\e[00;015mr12\e[0m=\e[00;012m%16p\e[0m r13=\e[00;12m%16p\e[0m r14=\e[00;12m%16p\e[0m\n"
              "\e[00;015mr15\e[0m=%16p \e[00;015mrip\e[0m=%16p \e[00;015mcr0\e[0m=%16p\n"
              "\e[00;015mcr2\e[0m=\e[00;012m%16p\e[0m cr3=\e[00;12m%16p\e[0m cr4=\e[00;12m%16p\e[0m\n"
              "\e[00;015mst\e[0m =%16p \e[00;015msp\e[0m =%16p \e[00;015mstz\e[0m=%16p\n",
              mctx->trapno, mctx, getcpuid(), getpid(), gettid(), *(uintptr_t *)mctx->rsp,
              mctx->errno,  mctx->rflags, mctx->cs,
              mctx->ds,     mctx->fs,     mctx->ss,
              mctx->rax,    mctx->rbx,    mctx->rcx,
              mctx->rdx,    mctx->rdi,    mctx->rsi,
              mctx->rbp,    mctx->rsp,    mctx->r8,
              mctx->r9,     mctx->r10,    mctx->r11,
              mctx->r12,    mctx->r13,    mctx->r14,
              mctx->r15,    mctx->rip,    rdcr0(),
              rdcr2(),      rdcr3(),      rdcr4(),
              stack_sp, stack_sp + stack_sz, stack_sz
        );
    } else {
        printk("\n\e[00;014mTRAP:%d\e[0m MCTX: %p CPU%d TID[%d:%d] to_ret->%p\n"
              "\e[00;015merr\e[0m=\e[00;012m%16p\e[0m rfl=\e[00;12m%16p\e[0m cs =\e[00;12m%16p\e[0m\n"
              "\e[00;015mds\e[0m =%16p \e[00;015mfs \e[0m=%16p \e[00;015mss \e[0m=%16p\n"
              "\e[00;015mrax\e[0m=\e[00;012m%16p\e[0m rbx=\e[00;12m%16p\e[0m rcx=\e[00;12m%16p\e[0m\n"
              "\e[00;015mrdx\e[0m=%16p \e[00;015mrdi\e[0m=%16p \e[00;015mrsi\e[0m=%16p\n"
              "\e[00;015mrbp\e[0m=\e[00;012m%16p\e[0m rsp=\e[00;12m%16p\e[0m r8 =\e[00;12m%16p\e[0m\n"
              "\e[00;015mr9\e[0m =%16p \e[00;015mr10\e[0m=%16p \e[00;015mr11\e[0m=%16p\n"
              "\e[00;015mr12\e[0m=\e[00;012m%16p\e[0m r13=\e[00;12m%16p\e[0m r14=\e[00;12m%16p\e[0m\n"
              "\e[00;015mr15\e[0m=%16p \e[00;015mrip\e[0m=%16p \e[00;015mcr0\e[0m=%16p\n"
              "\e[00;015mcr2\e[0m=\e[00;012m%16p\e[0m cr3=\e[00;12m%16p\e[0m cr4=\e[00;12m%16p\e[0m\n"
              "\e[00;015mst\e[0m =%16p \e[00;015msp\e[0m =%16p \e[00;015mstz\e[0m=%16p\n",
              mctx->trapno, mctx, getcpuid(), getpid(), gettid(), *(uintptr_t *)mctx->rsp,
              mctx->errno,  mctx->rflags, mctx->cs,
              mctx->ds,     mctx->fs,     mctx->ss,
              mctx->rax,    mctx->rbx,    mctx->rcx,
              mctx->rdx,    mctx->rdi,    mctx->rsi,
              mctx->rbp,    mctx->rsp,    mctx->r8,
              mctx->r9 ,    mctx->r10,    mctx->r11,
              mctx->r12,    mctx->r13,    mctx->r14,
              mctx->r15,    mctx->rip,    rdcr0(),
              rdcr2(),      rdcr3(),      rdcr4(),
              stack_sp, stack_sp + stack_sz, stack_sz
        );
    }
}

void dump_ctx(context_t *ctx, int halt) {
    if (halt) {
        panic(
            "\nctx: %p lnk: %p r11: %p\n"
            "r12: %p r13: %p r14: %p\n"
            "r15: %p rbp: %p rbx: %p rip: %p\n",
            ctx, ctx->link, ctx->r11,
            ctx->r12, ctx->r13, ctx->r14,
            ctx->r15, ctx->rbp, ctx->rbx, ctx->rip
        );
    } else {
        printk(
            "\nctx: %p lnk: %p r11: %p\n"
            "r12: %p r13: %p r14: %p\n"
            "r15: %p rbp: %p rbx: %p rip: %p\n",
            ctx, ctx->link, ctx->r11,
            ctx->r12, ctx->r13, ctx->r14,
            ctx->r15, ctx->rbp, ctx->rbx, ctx->rip
        );
    }
}

static void isr_ne(int trapno) {
    todo("trap(%d)\n", trapno);
}

void thread_handle_event(void) {
    if (!current)
        return;

    signal_dispatch();

    if (atomic_read(&current->t_info.ti_sched.ts_timeslice) == 0)
        sched_yield();
}

void trap(ucontext_t *uctx) {
    mcontext_t *mctx = &uctx->uc_mcontext;

    if (current) {
        current->t_arch.t_uctx = uctx;
        uctx->uc_stack = current_isuser() ? current->t_arch.t_kstack : current->t_arch.t_ustack;
        uctx->uc_link  = current->t_arch.t_uctx;
        current->t_arch.t_uctx = uctx;
        uctx->uc_flags = 0;
        sigsetempty(&uctx->uc_sigmask);
    }

    switch (mctx->trapno) {
    case T_LEG_SYSCALL:
        break;
    case T_PF:
        assert(0, "PF: %p\n", rdcr2());
    case T_PIT:
        break;
    case T_GP:
        dump_tf(mctx, 1);
        break;
    case T_PS2_KBD:
        ps2kbd_intr();
        break;
    case T_HPET:
        timer_intr();
        break;
    case T_RTC:
        break;
    case T_LAPIC_ERROR:
        break;
    case T_SPURIOUS:
    case T_LAPIC_SPURIOUS:
        break;
    case T_LAPIC_TIMER:
        lapic_timerintr();
        break;
    case T_TLBSHTDWN:
        break;
    case T_PANIC:
        isr_ne(mctx->trapno);
        break;
    case T_SIMTRAP:
        break;
    case T_LAPIC_IPI:
        printk("IPI: cpu:%d, tid: %d\n", getcpuid(), gettid());
        break;
    default:
        isr_ne(mctx->trapno);
    }

    if ((mctx->trapno >= IRQ_OFFSET) && (mctx->trapno < T_LEG_SYSCALL))
        lapic_eoi();
    
    thread_handle_event();

    if (current) {
        current->t_arch.t_uctx = uctx->uc_link;
    }
}