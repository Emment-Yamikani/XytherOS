#include <arch/traps.h>
#include <arch/ucontext.h>
#include <arch/x86_64/lapic.h>
#include <core/debug.h>
#include <dev/timer.h>
#include <sync/preempt.h>

static void isr_ne(int trapno) {
    todo("trap(%d)\n", trapno);
}

void trap(ucontext_t *uctx) {
    mcontext_t *mctx = &uctx->uc_mcontext;

    switch(mctx->trapno) {
        case T_PIT:
            break;
        case T_PS2_KBD:
            break;
        case T_HPET:
            timer_intr();
            printk("jiffies: %d\n", jiffies_get());
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
        case T_LAPIC_IPI:
            break;
        case T_PANIC:
            break;
        case T_LEG_SYSCALL:
            break;
        case T_SIMTRAP:
            break;
        default:
            isr_ne(mctx->trapno);
    }

    if (mctx->trapno >= IRQ_OFFSET)
        lapic_eoi();
}