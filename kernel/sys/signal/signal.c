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

/**
 * @brief Retrieve the signal handler for a given thread.
 * 
 * @param thread The thread whose signal handler is being requested.
 * @param signo The signal number.
 * @return The signal handler function pointer.
 */
__sighandler_t sig_handler(thread_t *thread, int signo);

/**
 * @brief Allocates memory for a new signal descriptor.
 * 
 * @param psp Pointer to store the allocated signal descriptor.
 * @return 0 on success, negative error code on failure.
 */
int signal_alloc(signal_t **psp);

/**
 * @brief Frees the memory allocated for a signal descriptor.
 * 
 * @param sigdesc Pointer to the signal descriptor to free.
 */
void signal_free(signal_t *sigdesc);

/**
 * @brief Frees memory allocated for a signal information structure.
 * 
 * @param siginfo Pointer to the signal information structure.
 */
void siginfo_free(siginfo_t *siginfo);

/**
 * @brief Initializes a siginfo structure with signal details.
 * 
 * @param siginfo Pointer to the siginfo structure to initialize.
 * @param signo The signal number.
 * @param val Signal value (additional data).
 * @return 0 on success, -EINVAL if the signal number is invalid.
 */
int siginfo_init(siginfo_t *siginfo, int signo, union sigval val);

/**
 * @brief Allocates memory for a siginfo structure.
 * 
 * @param psiginfo Pointer to store the allocated siginfo structure.
 * @return 0 on success, negative error code on failure.
 */
int siginfo_alloc(siginfo_t **psiginfo);

/**
 * @brief Prints the contents of a siginfo structure.
 * 
 * @param siginfo Pointer to the siginfo structure.
 */
void siginfo_dump(siginfo_t *siginfo);

/**
 * @brief Enqueues a signal in the process's pending signal queue.
 * 
 * @param sigdesc The signal descriptor of the target process.
 * @param siginfo The signal information structure.
 * @return 0 on success, negative error code on failure.
 */
int signal_enqueue(signal_t *sigdesc, siginfo_t *siginfo);

/**
 * @brief Dequeues the next pending signal from the process's signal queue.
 * 
 * @param sigdesc The signal descriptor of the target process.
 * @param psiginfo Pointer to store the dequeued signal information.
 * @return 0 on success, -ENOENT if no signals are pending.
 */
int signal_dequeue(signal_t *sigdesc, siginfo_t **psiginfo);

/**
 * @brief Enqueues a signal into a queue.
 * 
 * @param sigqueue The queue to enqueue the signal into.
 * @param siginfo The signal information to enqueue.
 * @return 0 on success, negative error code on failure.
 */
int sigqueue_enqueue(queue_t *sigqueue, siginfo_t *siginfo);

/**
 * @brief Dequeues a signal from a queue.
 * 
 * @param sigqueue The queue to dequeue from.
 * @param psiginfo Pointer to store the dequeued signal information.
 * @return 0 on success, negative error code on failure.
 */
int sigqueue_dequeue(queue_t *sigqueue, siginfo_t **psiginfo);

/**
 * @brief Flushes all signals from a signal queue.
 * 
 * @param sigqueue The queue to flush.
 */
void sigqueue_flush(queue_t *sigqueue);

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

    sigemptyset(&sigdesc->sig_mask);

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

    if ((err = sigaddset(&sigdesc->sigpending, siginfo->si_signo)))
        return err;

    queue_lock(&sigdesc->sig_queue[siginfo->si_signo - 1]);
    if ((err = sigqueue_enqueue(&sigdesc->sig_queue[siginfo->si_signo - 1], siginfo))) {
        if (!queue_count(&sigdesc->sig_queue[siginfo->si_signo])) {
            sigdelset(&sigdesc->sigpending, siginfo->si_signo);
        }
    }
    queue_unlock(&sigdesc->sig_queue[siginfo->si_signo - 1]);
    return err;
}

int signal_dequeue(signal_t *sigdesc, siginfo_t **psiginfo) {
    if (sigdesc == NULL || psiginfo == NULL)
        return -EINVAL;

    signal_assert_locked(sigdesc);

    for (usize signo = 0; signo < NSIG; ++signo) {
        if (sigismember(&sigdesc->sigpending, signo + 1)) {
            if (!sigismember(&sigdesc->sig_mask, signo + 1)) {
                queue_lock(&sigdesc->sig_queue[signo]);
                int err = sigqueue_dequeue(&sigdesc->sig_queue[signo], psiginfo);
                if (!queue_count(&sigdesc->sig_queue[signo])) {
                    sigdelset(&sigdesc->sigpending, signo + 1);
                }
                queue_unlock(&sigdesc->sig_queue[signo]);
                return err;
            }
        }
    }

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
