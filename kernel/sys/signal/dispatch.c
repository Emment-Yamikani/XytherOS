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

void signal_dispatch(void) {
    siginfo_t       *siginfo    = NULL;
    __sighandler_t  handler     = NULL;

    signal_lock(current->t_signals);
    if (signal_dequeue(current, &siginfo)) {
        signal_unlock(current->t_signals);
        return;
    }

    handler = sig_handler(current, siginfo->si_signo);

    if (sig_handler_ignored(handler, siginfo->si_signo)) { // Signal ignored explicitly or implicitly?
        do_ignore(siginfo);
    } else if (handler == SIG_DFL) { // Take default action?
        do_default_action(siginfo);
    }

    arch_signal_dispatch(
        &current->t_arch, (thread_entry_t)handler, siginfo,
        &current->t_signals->sig_action[siginfo->si_signo - 1]
    );

    signal_unlock(current->t_signals);
    siginfo_free(siginfo);
}