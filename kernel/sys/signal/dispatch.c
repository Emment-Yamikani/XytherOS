#include <bits/errno.h>
#include <core/debug.h>
#include <sys/thread.h>

static void do_ignore(siginfo_t *siginfo) {
    debug("Ignoring: %s\n", signal_str[siginfo->si_signo - 1]);
}

static void do_default_action(siginfo_t *siginfo) {
    assert(siginfo, "Invalid siginfo.\n");

    if (sig_abort(siginfo->si_signo)) {
        debug("abort: %s\n", signal_str[siginfo->si_signo - 1]);
    } else if (sig_terminate(siginfo->si_signo)) {
        debug("terminate: %s\n", signal_str[siginfo->si_signo - 1]);
    } else if (sig_term_dumpcore(siginfo->si_signo)) {
        debug("term_dumpcore: %s\n", signal_str[siginfo->si_signo - 1]);
    } else if (sig_stop(siginfo->si_signo)) {
        debug("stop: %s\n", signal_str[siginfo->si_signo - 1]);
    } else if (sig_continue(siginfo->si_signo)) {
        debug("continue: %s\n", signal_str[siginfo->si_signo - 1]);
    }
}

static void signal_mask_block(int signo, sigaction_t *act, sigset_t *oset) {
    sigset_t set;
    
    sigsetempty(&set);

    // Block delivery of signo to this thread while we handle it.
    if (!(act->sa_flags & SA_NODEFER)) {
        sigsetadd(&set, signo);
    }

    // Block all signals that are set in sa_mask.
    sigsetaddsetmask(&set, &act->sa_mask);

    // Block signals specified in 'set'.
    sigmask(&current->t_sigmask, SIG_BLOCK, &set, oset);
}

static void signal_mask_restore(sigset_t *oset) {
    sigmask(&current->t_sigmask, SIG_SETMASK, oset, NULL);
}

void signal_dispatch(void) {
    sigaction_t     oact;
    siginfo_t       *siginfo    = NULL;
    __sighandler_t  handler     = NULL;
    arch_thread_t   *arch       = &current->t_arch;

    if (arch->t_nsig_nested > ARCH_NSIG_NESTED) {
        return;
    }

    current_lock();
    if (signal_dequeue(current, &oact, &siginfo)) {
        current_unlock();
        return;
    }

    arch->t_nsig_nested++;

    // block the set of signals we don't want to interrupt this context
    signal_mask_block(siginfo->si_signo, &oact, &arch->t_uctx->uc_sigmask);

    handler = oact.sa_handler;
    if (sig_handler_ignored(handler, siginfo->si_signo)) { // Signal ignored explicitly or implicitly?
        do_ignore(siginfo);
        goto done;
    } else if (handler == SIG_DFL) { // Take default action?
        do_default_action(siginfo);
        goto done;
    }

    arch_signal_dispatch(&current->t_arch, &oact, siginfo);
done:
    signal_mask_restore(&arch->t_uctx->uc_sigmask); // restore old signal set.
    current_unlock();
    if (siginfo)
        siginfo_free(siginfo);
    
    arch->t_nsig_nested--;
}