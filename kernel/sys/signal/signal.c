/**
 * @file signal.c
 * @brief Signal handling implementation for xytherOS.
 * 
 * Provides functions for allocating, enqueuing, dequeuing, and processing signals
 * for threads within the kernel.
 */

#include <bits/errno.h>
#include <core/debug.h>
#include <mm/kalloc.h>
#include <string.h>
#include <sys/thread.h>

const char *signal_str[] = {
    [SIGABRT - 1]   = "SIGABRT",
    [SIGALRM - 1]   = "SIGALRM",
    [SIGBUS - 1]    = "SIGBUS",
    [SIGCANCEL - 1] = "SIGCANCEL",
    [SIGCHLD - 1]   = "SIGCHLD",
    [SIGCONT - 1]   = "SIGCONT",
    [SIGEMT - 1]    = "SIGEMT",
    [SIGFPE - 1]    = "SIGFPE",
    [SIGHUP - 1]    = "SIGHUP",
    [SIGILL - 1]    = "SIGILL",
    [SIGINT - 1]    = "SIGINT",
    [SIGIO - 1]     = "SIGIO",
    [SIGIOT - 1]    = "SIGIOT",
    [SIGKILL - 1]   = "SIGKILL",
    [SIGPIPE - 1]   = "SIGPIPE",
    [SIGPROF - 1]   = "SIGPROF",
    [SIGQUIT - 1]   = "SIGQUIT",
    [SIGSEGV - 1]   = "SIGSEGV",
    [SIGSTOP - 1]   = "SIGSTOP",
    [SIGSYS - 1]    = "SIGSYS",
    [SIGTERM - 1]   = "SIGTERM",
    [SIGTRAP - 1]   = "SIGTRAP",
    [SIGTSTP - 1]   = "SIGTSTP",
    [SIGTTIN - 1]   = "SIGTTIN",
    [SIGTTOU - 1]   = "SIGTTOU",
    [SIGURG - 1]    = "SIGURG",
    [SIGUSR1 - 1]   = "SIGUSR1",
    [SIGUSR2 - 1]   = "SIGUSR2",
    [SIGVTALRM - 1] = "SIGVTALRM",
    [SIGWINCH - 1]  = "SIGWINCH",
    [SIGXCPU - 1]   = "SIGXCPU",
    [SIGXFSZ - 1]   = "SIGXFSZ",
};

const int sig_defaults[] = {
    [SIGABRT - 1]   = SIG_TERM_CORE,    // | terminate+core
    [SIGALRM - 1]   = SIG_TERM,         // | terminate
    [SIGBUS - 1]    = SIG_TERM_CORE,    // | terminate+core
    [SIGCANCEL - 1] = SIG_IGNORE,       // | ignore
    [SIGCHLD - 1]   = SIG_IGNORE,       // | ignore
    [SIGCONT - 1]   = SIG_CONT,         // | continue/ignore
    [SIGEMT - 1]    = SIG_TERM_CORE,    // | terminate+core
    [SIGFPE - 1]    = SIG_TERM_CORE,    // | terminate+core
    [SIGHUP - 1]    = SIG_TERM,         // | terminate
    [SIGILL - 1]    = SIG_TERM_CORE,    // | terminate+core
    [SIGINT - 1]    = SIG_TERM,         // | terminate
    [SIGIO - 1]     = SIG_TERM,         // | terminate/ignore
    [SIGIOT - 1]    = SIG_TERM_CORE,    // | terminate+core
    [SIGKILL - 1]   = SIG_TERM,         // | terminate
    [SIGPIPE - 1]   = SIG_TERM,         // | terminate
    [SIGPROF - 1]   = SIG_TERM,         // | terminate
    [SIGQUIT - 1]   = SIG_TERM_CORE,    // | terminate+core
    [SIGSEGV - 1]   = SIG_TERM_CORE,    // | terminate+core
    [SIGSTOP - 1]   = SIG_STOP,         // | stop process
    [SIGSYS - 1]    = SIG_TERM_CORE,    // | terminate+core
    [SIGTERM - 1]   = SIG_TERM,         // | terminate
    [SIGTRAP - 1]   = SIG_TERM_CORE,    // | terminate+core
    [SIGTSTP - 1]   = SIG_STOP,         // | stop process
    [SIGTTIN - 1]   = SIG_STOP,         // | stop process
    [SIGTTOU - 1]   = SIG_STOP,         // | stop process
    [SIGURG - 1]    = SIG_IGNORE,       // | ignore
    [SIGUSR1 - 1]   = SIG_TERM,         // | terminate
    [SIGUSR2 - 1]   = SIG_TERM,         // | terminate
    [SIGVTALRM - 1] = SIG_TERM,         // | terminate
    [SIGWINCH - 1]  = SIG_IGNORE,       // | ignore
    [SIGXCPU - 1]   = SIG_TERM,         // | teminate or terminate+core
    [SIGXFSZ - 1]   = SIG_TERM,         // | teminate or terminate+core
};

__sighandler_t sig_handler(thread_t *thread, int signo) {
    signal_assert_locked(thread->t_signals);
    return thread->t_signals->sig_action[signo - 1].sa_handler;
}

int signal_alloc(signal_t **psp) {
    int         err;
    signal_t   *sigdesc;

    if (psp == NULL)
        return -EINVAL;

    if (NULL == (sigdesc = kmalloc(sizeof *sigdesc)))
        return -ENOMEM;

    memset(sigdesc, 0, sizeof *sigdesc);

    sigsetempty(&sigdesc->sig_mask);

    for (usize signo = 0; signo < NSIG; ++signo) {
        if ((err = queue_init(&sigdesc->sig_queue[signo]))) {
            kfree(sigdesc);
            return err;
        }
        sigdesc->sig_action[signo].sa_handler = SIG_DFL;
    }

    spinlock_init(&sigdesc->sig_lock);

    *psp = sigdesc;

    return 0;
}

void signal_free(signal_t *sigdesc) {
    assert(sigdesc, "Invalid sigdesc.\n");

    spin_recursive_lock(&sigdesc->sig_lock);

    for (usize signo = 0; signo < NSIG; ++signo) {
        queue_lock(&sigdesc->sig_queue[signo]);
        queue_flush(&sigdesc->sig_queue[signo]);
        queue_unlock(&sigdesc->sig_queue[signo]);

        sigdesc->sig_action[signo].sa_handler = SIG_DFL;
    }

    spin_unlock(&sigdesc->sig_lock);

    kfree(sigdesc);
}

void siginfo_free(siginfo_t *siginfo) {
    assert(siginfo, "Invalid siginfo.\n");
    kfree(siginfo);
}

int siginfo_init(siginfo_t *siginfo, int signo, union sigval val) {
    if (siginfo == NULL || SIGBAD(signo))
        return -EINVAL;

    siginfo->si_value = val;
    siginfo->si_signo = signo;
    siginfo->si_pid   = getpid();

    return 0;
}

int siginfo_alloc(siginfo_t **psiginfo) {
    siginfo_t *siginfo;

    if (psiginfo == NULL)
        return -EINVAL;

    if (NULL == (siginfo = kmalloc(sizeof *siginfo)))
        return -ENOMEM;

    memset(siginfo, 0, sizeof *siginfo);

    *psiginfo = siginfo;

    return 0;
}

void siginfo_dump(siginfo_t *siginfo) {
    assert(siginfo, "Invalid siginfo.\n");

    printk(
        "si_signo:  %d\n"
        "si_code:   %d\n"
        "si_pid:    %d\n"
        "si_uid:    %d\n"
        "si_addr:   %p\n"
        "si_status: %p\n"
        "si_value:  %p\n",
        siginfo->si_signo,
        siginfo->si_code,
        siginfo->si_pid,
        siginfo->si_uid,
        siginfo->si_addr,
        siginfo->si_status,
        siginfo->si_value.sigval_ptr
    );
}

int signal_enqueue(signal_t *sigdesc, siginfo_t *siginfo) {
    int err;

    if (sigdesc == NULL || siginfo == NULL || SIGBAD(siginfo->si_signo))
        return -EINVAL;

    signal_assert_locked(sigdesc);

    if ((err = sigsetadd(&sigdesc->sigpending, siginfo->si_signo)))
        return err;

    queue_lock(&sigdesc->sig_queue[siginfo->si_signo - 1]);
    if ((err = sigqueue_enqueue(&sigdesc->sig_queue[siginfo->si_signo - 1], siginfo))) {
        if (!queue_count(&sigdesc->sig_queue[siginfo->si_signo - 1])) {
            sigsetdel(&sigdesc->sigpending, siginfo->si_signo);
        }
    }
    queue_unlock(&sigdesc->sig_queue[siginfo->si_signo - 1]);
    return err;
}

/**
 * Helper function to try to dequeue a pending signal.
 *
 * This function checks whether a given signal is pending and not blocked,
 * locks the corresponding signal queue, attempts to dequeue the signal, and
 * updates the pending set and thread’s signal mask as needed.
 *
 * @param thread[in]        The thread or process for whom signals maybe pending.
 * @param signo[in]         The thread’s signal mask (to block the signal upon dequeue).
 * @param proc_level[in]    The signal level to dequeue (either 'false' for thread or 'true' process level).
 * @param psiginfo[out]     Output pointer for the dequeued signal information.
 *
 * @return 0 on success, or an error code (e.g. -ENOENT if not eligible).
 */
static int try_dequeue_signal(thread_t *thread, int signo, bool proc_level, siginfo_t **psiginfo) {
    int         err     = 0;
    queue_t     *queue  = NULL;
    sigset_t    *pending= NULL;

    pending = proc_level ? &thread->t_signals->sigpending : &thread->t_sigpending;
    if (!sigismember(pending, signo) || sigismember(&thread->t_sigmask, signo))
        return -ENOENT;

    queue = proc_level ? &thread->t_signals->sig_queue[signo - 1] : &thread->t_sigqueue[signo - 1];
    queue_lock(queue);
    err = sigqueue_dequeue(queue, psiginfo);
    if (err == 0){
        if (!queue_count(queue))
            sigsetdel(pending, signo);
        sigsetadd(&thread->t_sigmask, signo);
        thread->t_arch.t_uctx->uc_sigmask = thread->t_sigmask;
    }
    queue_unlock(queue);
    return err;
}

/**
 * Dequeues a pending signal for the given thread.
 *
 * This function first checks the thread-specific pending signals and then the
 * process-wide pending signals. In both cases, if an eligible signal is found,
 * it is dequeued and the signal is blocked in the thread’s mask.
 *
 * @param thread   Pointer to the thread structure.
 * @param psiginfo Output pointer that will be set to the dequeued signal info.
 *
 * @return 0 on success, or an error code (e.g., -ENOENT if no signal is found).
 */
int signal_dequeue(thread_t *thread, siginfo_t **psiginfo) {
    int         err     = 0;
    siginfo_t   *siginfo= NULL;

    if (thread == NULL || psiginfo == NULL)
        return -EINVAL;

    /* Lock the thread to safely access its signal data */
    thread_lock(thread);

    for (int signo = 1; signo <= NSIG; ++signo) {
        err = try_dequeue_signal(thread, signo, false, &siginfo);
        if (err != -ENOENT) {
            *psiginfo = siginfo;
            thread_unlock(thread);
            return err;
        }
    }

    /* Next, check process-wide pending signals. */
    for (int signo = 1; signo <= NSIG; ++signo) {
        err = try_dequeue_signal(thread, signo, true, &siginfo);
        if (err != -ENOENT) {
            *psiginfo = siginfo;
            thread_unlock(thread);
            return err;
        }
    }

    thread_unlock(thread);
    return -ENOENT;
}

int sigqueue_enqueue(queue_t *sigqueue, siginfo_t *siginfo) {
    if (sigqueue == NULL || siginfo == NULL)
        return -EINVAL;
    queue_assert_locked(sigqueue);
    return enqueue(sigqueue, siginfo, QUEUE_ENFORCE_UNIQUE, NULL);
}

int sigqueue_dequeue(queue_t *sigqueue, siginfo_t **psiginfo) {
    if (sigqueue == NULL || psiginfo == NULL)
        return -EINVAL;
    queue_assert_locked(sigqueue);
    return dequeue(sigqueue, (void **)psiginfo);
}

void sigqueue_flush(queue_t *sigqueue) {
    queue_assert_locked(sigqueue);
    queue_foreach(sigqueue, siginfo_t *, siginfo) {
        queue_remove_node(sigqueue, siginfo_node);
        siginfo_free(siginfo);
    }
}
