#include <arch/traps.h>
#include <arch/ucontext.h>
#include <arch/x86_64/lapic.h>
#include <core/debug.h>
#include <dev/ps2.h>
#include <dev/timer.h>
#include <sys/schedule.h>
#include <sys/thread.h>
#include <sync/preempt.h>
#include <sys/_syscall.h>

void dump_tf(mcontext_t *mctx, int halt) {
    usize  stack_sz = 0;
    uintptr_t stack_sp = 0;

    if (current) {
        stack_sp = (uintptr_t)current->t_arch.t_kstack.ss_sp;
        stack_sz = current->t_arch.t_kstack.ss_size;
    }

    dumpf(halt,
        "\e[34mTRAP\e[0m:\e[34m%d\e[0m CPU%d TID[%d:%d] ret[\e[92m%p\e[0m]\n"
        "\e[93merr\e[0m=%s%16p\e[0m \e[93mrfl\e[0m=%s%16p\e[0m \e[93mcs\e[0m =%s%16p\e[0m\n"
        "\e[37mds\e[0m =%s%16p\e[0m \e[37mfs\e[0m =%s%16p\e[0m \e[37mss\e[0m =%s%16p\e[0m\n"
        "\e[93mrax\e[0m=%s%16p\e[0m \e[93mrbx\e[0m=%s%16p\e[0m \e[93mrcx\e[0m=%s%16p\e[0m\n"
        "\e[37mrdx\e[0m=%s%16p\e[0m \e[37mrdi\e[0m=%s%16p\e[0m \e[37mrsi\e[0m=%s%16p\e[0m\n"
        "\e[93mrbp\e[0m=%s%16p\e[0m \e[93mrsp\e[0m=%s%16p\e[0m \e[93mr8\e[0m =%s%16p\e[0m\n"
        "\e[37mr9\e[0m =%s%16p\e[0m \e[37mr10\e[0m=%s%16p\e[0m \e[37mr11\e[0m=%s%16p\e[0m\n"
        "\e[93mr12\e[0m=%s%16p\e[0m \e[93mr13\e[0m=%s%16p\e[0m \e[93mr14\e[0m=%s%16p\e[0m\n"
        "\e[37mr15\e[0m=%s%16p\e[0m \e[37mrip\e[0m=%s%16p\e[0m \e[37mcr0\e[0m=%s%16p\e[0m\n"
        "\e[93mcr2\e[0m=%s%16p\e[0m \e[93mcr3\e[0m=%s%16p\e[0m \e[93mcr4\e[0m=%s%16p\e[0m\n"
        "\e[37mstk\e[0m=%s%16p\e[0m \e[37mstp\e[0m=%s%16p\e[0m \e[37mstz\e[0m=%s%16p\e[0m\n",
        mctx->trapno, getcpuid(), getpid(), gettid(), *(uintptr_t *)mctx->rbp,
        "\e[33m", mctx->eno, "\e[33m", mctx->rfl, "\e[33m", mctx->cs,
        "\e[92m", mctx->ds, "\e[92m", mctx->fs, "\e[92m", mctx->ss,
        "\e[33m", mctx->rax, "\e[33m", mctx->rbx, "\e[33m", mctx->rcx,
        "\e[92m", mctx->rdx, "\e[92m", mctx->rdi, "\e[92m", mctx->rsi,
        "\e[33m", mctx->rbp, "\e[33m", mctx->rsp, "\e[33m", mctx->r8,
        "\e[92m", mctx->r9, "\e[92m", mctx->r10, "\e[92m", mctx->r11,
        "\e[33m", mctx->r12, "\e[33m", mctx->r13, "\e[33m", mctx->r14,
        "\e[92m", mctx->r15, "\e[92m", mctx->rip, "\e[92m", rdcr0(),
        "\e[96m", mctx->cr2, "\e[96m", rdcr3(), "\e[96m", rdcr4(),
        "\e[91m", stack_sp, "\e[91m", stack_sp + stack_sz, "\e[91m", stack_sz
    );
}

void dump_ctx(context_t *ctx, int halt) {
    dumpf(halt, "\n\t\t\t\t\t\tctx: %p\n"
        "lnk: %p r11: %p r12: %p\n"
        "r13: %p r14: %p r15: %p\n"
        "rbp: %p rbx: %p rip: %p\n",
        ctx, ctx->link, ctx->r11,
        ctx->r12, ctx->r13, ctx->r14,
        ctx->r15, ctx->rbp, ctx->rbx, ctx->rip
    );
}

static void isr_ne(int trapno) {
    todo("trap(%d)\n", trapno);
}

void thread_handle_event(void) {
    if (current == NULL) {
        return;
    }

    signal_dispatch();

    if (current_gettimeslice() == 0) {
        sched_yield();
    }

    current->t_arch.t_uctx = current->t_arch.t_uctx->uc_link;
}

// End of interrupt.
static inline void eointr(mcontext_t *mctx) {
    if (mctx->trapno < IRQ_OFFSET) {
        return;
    }

    if (mctx->trapno >= T_LEG_SYSCALL) {
        return;
    }

    lapic_eoi();
}

static inline void do_link_ucontext(ucontext_t *uctx) {
    if (current == NULL) {
        return;
    }

    thread_get_uc_stack(current, &uctx->uc_stack);

    uctx->uc_link = current->t_arch.t_uctx;

    current->t_arch.t_uctx = uctx;

    uctx->uc_flags = 0;
}

void trap(ucontext_t *uctx) {
    mcontext_t *mctx = &uctx->uc_mcontext;

    do_link_ucontext(uctx);

    switch (mctx->trapno) {
        case T_LEG_SYSCALL: do_syscall(uctx); break;

        case T_PF: arch_do_page_fault(mctx); break;

        case T_PIT: break;

        case T_GP: dump_tf(mctx, 1); break;

        case T_PS2_KBD: ps2kbd_intr(); break;

        case T_HPET: timer_intr(); break;

        case T_RTC: break;

        case T_NM: nm_except(); break;

        case T_MF: mf_except(); break;

        case T_XM: smid_except(); break;

        case T_LAPIC_ERROR: break;

        case T_SPURIOUS: case T_LAPIC_SPURIOUS: break; // spurious interrupt.

        case T_LAPIC_TIMER: lapic_timerintr(); break;

        case T_TLBSHTDWN: break;

        case T_PANIC: isr_ne(mctx->trapno); break;

        case T_SIMTRAP: break;

        case T_LAPIC_IPI:
            printk("IPI: cpu:%d, tid: %d\n", getcpuid(), gettid());
            break;
        default: isr_ne(mctx->trapno);
    }

    eointr(mctx);

    thread_handle_event();
}