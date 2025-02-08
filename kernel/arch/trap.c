#include <arch/traps.h>
#include <arch/ucontext.h>
#include <arch/x86_64/lapic.h>
#include <core/debug.h>
#include <sync/preempt.h>

static isr_ne(int trapno) {
    todo("trap(%d)\n", trapno);
}

void trap(ucontext_t *uctx) {
    mcontext_t *mctx = &uctx->uc_mcontext;

    switch(mctx->trapno) {
        case T_DE:
            isr_ne(mctx->trapno);
            break;
        case T_DB:
            isr_ne(mctx->trapno);
            break;
        case T_NMI:
            isr_ne(mctx->trapno);
            break;
        case T_BP:
            isr_ne(mctx->trapno);
            break;
        case T_OF:
            isr_ne(mctx->trapno);
            break;
        case T_BR:
            isr_ne(mctx->trapno);
            break;
        case T_UD:
            isr_ne(mctx->trapno);
            break;
        case T_NM:
            isr_ne(mctx->trapno);
            break;
        case T_DF:
            isr_ne(mctx->trapno);
            break;
        case T_CSO:
            isr_ne(mctx->trapno);
            break;
        case T_TS:
            isr_ne(mctx->trapno);
            break;
        case T_NP:
            isr_ne(mctx->trapno);
            break;
        case T_SS:
            isr_ne(mctx->trapno);
            break;
        case T_GP:
            isr_ne(mctx->trapno);
            break;
        case T_PF:
            isr_ne(mctx->trapno);
            break;
        case T_MF:
            isr_ne(mctx->trapno);
            break;
        case T_AC:
            isr_ne(mctx->trapno);
            break;
        case T_MC:
            isr_ne(mctx->trapno);
            break;
        case T_XM:
            isr_ne(mctx->trapno);
            break;
        case T_VE:
            isr_ne(mctx->trapno);
            break;

        case T_PIT:
            break;
        case T_PS2_KBD:
            break;
        case T_HPET:
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