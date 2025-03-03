#include <arch/cpu.h>
#include <bits/errno.h>
#include <core/debug.h>
#include <string.h>
#include <sys/schedule.h>
#include <sys/thread.h>

int x86_64_signal_dispatch(arch_thread_t *arch, sigaction_t *act, siginfo_t *siginfo) {
    if (!arch || !arch->t_thread || !act || !act->sa_handler || !siginfo)
        return -EINVAL;

    if (act->sa_flags & SA_ONSTACK) {
        if (!arch->t_altstack.ss_sp || !arch->t_altstack.ss_size)
            return -EINVAL;

        if (arch->t_altstack.ss_flags & SS_DISABLE)
            goto __no_altsigstk;

        
    }

__no_altsigstk:
    if (act->sa_flags & SA_SIGINFO)
        act->sa_handler(siginfo->si_signo, siginfo, arch->t_uctx);
    else act->sa_handler(siginfo->si_signo);
    return 0;
}