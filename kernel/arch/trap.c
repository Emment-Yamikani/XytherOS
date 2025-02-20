#include <arch/traps.h>
#include <arch/ucontext.h>
#include <arch/x86_64/lapic.h>
#include <core/debug.h>
#include <dev/ps2.h>
#include <dev/timer.h>
#include <sys/schedule.h>
#include <sys/thread.h>
#include <sync/preempt.h>

static void isr_ne(int trapno) {
    todo("trap(%d)\n", trapno);
}

void thread_handle_even(ucontext_t *uctx) {
    if (!uctx || !current)
        return;

    if (atomic_read(&current->t_info.ti_sched.ts_timeslice) == 0)
        sched_yield();
}

void trap(ucontext_t *uctx) {
    mcontext_t *mctx = &uctx->uc_mcontext;

    if (current) {
    }

    thread_handle_even(uctx);

    switch (mctx->trapno) {
    case T_LEG_SYSCALL:
        break;
    case T_PIT:
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

    thread_handle_even(uctx);
}