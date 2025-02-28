#pragma once

#include <bits/errno.h>
#include <core/types.h>
#include <sync/spinlock.h>
#include <ds/queue.h>

#define SIGABRT     1  // abnormal termination (abort)         | terminate+core.
#define SIGALRM     2  // timer expired (alarm)                | terminate.
#define SIGBUS      3  // hardware fault                       | terminate+core.
#define SIGCANCEL   4  // threads library internal use         | ignore.
#define SIGCHLD     5  // change in status of child            | ignore.
#define SIGCONT     6  // continue stopped process             | continue/ignore.
#define SIGEMT      7  // hardware fault                       | terminate+core.
#define SIGFPE      8  // arithmetic exception                 | terminate+core.
#define SIGHUP      9  // hangup                               | terminate.
#define SIGILL      10 // illegal instruction                  | terminate+core.
#define SIGINT      11 // terminal interrupt character         | terminate.
#define SIGIO       12 // asynchronous I/O                     | terminate/ignore.
#define SIGIOT      13 // hardware fault                       | terminate+core.
#define SIGKILL     14 // termination                          | terminate.
#define SIGPIPE     15 // write to pipe with no readers        | terminate.
#define SIGPROF     16 // profiling time alarm (setitimer)     | terminate.
#define SIGQUIT     17 // terminal quit character              | terminate+core.
#define SIGSEGV     18 // invalid memory reference             | terminate+core.
#define SIGSTOP     19 // stop                                 | stop process.
#define SIGSYS      20 // invalid system call                  | terminate+core.
#define SIGTERM     21 // termination                          | terminate.
#define SIGTRAP     22 // hardware fault                       | terminate+core.
#define SIGTSTP     23 // terminal stop character              | stop process.
#define SIGTTIN     24 // background read from control tty     | stop process.
#define SIGTTOU     25 // background write to control tty      | stop process.
#define SIGURG      26 // urgent condition (sockets)           | ignore.
#define SIGUSR1     27 // user-defined signal                  | terminate.
#define SIGUSR2     28 // user-defined signal                  | terminate.
#define SIGVTALRM   29 // virtual time alarm (setitimer)       | terminate.
#define SIGWINCH    30 // terminal window size change          | ignore.
#define SIGXCPU     31 // CPU limit exceeded (setrlimit)       | terminate or terminate+core.
#define SIGXFSZ     32 // file size limit exceeded (setrlimit) | terminate or terminate+core.

#define NSIG        32

/**
 * @brief Action to be taken when a signal arrives.
 * argument list left empty because this will be used as sa_handler or sa_sigaction*/
typedef void        (*__sighandler_t)();

#define SIG_ERR     ((__sighandler_t) -1)  /* error setting signal disposition. */
#define SIG_DFL     ((__sighandler_t) 0)   /* default action taken. */
#define SIG_IGN     ((__sighandler_t) 1)   /* ignore this signal. */

/** DEFAULT ACTION FOR SINGNAL(n) */

#define SIG_IGNORE      (1)
#define SIG_ABRT        (2)
#define SIG_TERM        (3)
#define SIG_TERM_CORE   (4)
#define SIG_STOP        (5)
#define SIG_CONT        (6)

extern const char *signal_str[];

extern const int sig_defaults[];

typedef unsigned long sigset_t;
typedef sigset_t __sigset_t;

#define SIGBAD(signo) ({ ((signo) < 1 || (signo) > NSIG); })

#define SIGMASK(signo)  (1ul << (signo - 1))

static inline int sigemptyset(sigset_t *set) {
    if ((set) == NULL)
        return -EINVAL;
    *set = (sigset_t)0;
    return 0;
}

static inline int sigfillset(sigset_t *set) {
    if ((set) == NULL)
        return -EINVAL;
    *set = ~(sigset_t)0;
    return 0;
}

static inline int sigaddset(sigset_t *set, int signo) {
    if ((set) == NULL || SIGBAD(signo))
        return -EINVAL;
    *set |= (sigset_t)(1 << (signo-1));
    return 0;
}

static inline int sigdelset(sigset_t *set, int signo) {
    if ((set) == NULL || SIGBAD(signo))
        return -EINVAL;
    *set &= ~(sigset_t)(1 << (signo-1));
    return 0;
}

static inline void sigdelsetmask(sigset_t *set, uint mask) {
    *set &= ~(sigset_t)mask;
}

static inline int sigismember(const sigset_t *set, int signo) {
    if ((set) == NULL || SIGBAD(signo))
        return -EINVAL;
    return (*set & ((sigset_t)(1 << (signo - 1)))) ? 1 : 0;
}

#define SIG_BLOCK   (1)
#define SIG_UNBLOCK (2)
#define SIG_SETMASK (3)

/*
If signo is SIGCHLD, do not generate this signal
when a child process stops (job control). This
signal is still generated, of course, when a child
terminates (but see the SA_NOCLDWAIT option
below).
*/
#define SA_NOCLDSTOP    BS(0)

/*
If signo is SIGCHLD, this option prevents the
system from creating zombie processes when
children of the calling process terminate. If it
subsequently calls wait, the calling process
blocks until all its child processes have
terminated and then returns −1 with errno set
to ECHILD.
*/
#define SA_NOCLDWAIT    BS(1)

/*
When this signal is caught, the signal is not
automatically blocked by the system while the
signal-catching function executes (unless the
signal is also included in sa_mask). Note that
this type of operation corresponds to the earlier
unreliable signals.
*/
#define SA_NODEFER      BS(2)

/*
If an alternative stack has been declared with
sigaltstack(2), this signal is delivered to the
process on the alternative stack.
*/
#define SA_ONSTACK      BS(3)

/*
The disposition for this signal is reset to
SIG_DFL, and the SA_SIGINFO flag is cleared
on entry to the signal-catching function. Note
that this type of operation corresponds to the
earlier unreliable signals. The disposition for
the two signals SIGILL and SIGTRAP can’t be
reset automatically, however. Setting this flag
can optionally cause sigaction to behave as if
SA_NODEFER is also set.
*/
#define SA_RESETHAND    BS(4)

/*
System calls interrupted by this signal are
automatically restarted.
*/
#define SA_RESTART      BS(5)

/*
This option provides additional information to a
signal handler: a pointer to a siginfo structure
and a pointer to an identifier for the process
context.
*/
#define SA_SIGINFO      BS(6)

union sigval {
    int             sigval_int;     /* Integer value */
    void            *sigval_ptr;    /* Pointer value */
};

typedef struct {
    int             si_signo;   /* Signal number */
    int             si_code;    /* Signal code */
    pid_t           si_pid;     /* Sending process ID */
    uid_t           si_uid;     /* Real user ID of sending process */
    void            *si_addr;   /* Address of faulting instruction */
    int             si_status;  /* Exit value or signal */
    union sigval    si_value;   /* Signal value */
} siginfo_t;

typedef struct __uc_stack_t {
    void    *ss_sp;     /* stack base or pointer */
    size_t  ss_size;    /* stack size */
    i32     ss_flags;   /* flags */
} __packed uc_stack_t;

typedef struct sigaction {
    __sighandler_t  sa_handler; /* addr of signal handler, SIG_IGN, or SIG_DFL */
    int             sa_flags;   /* signal options */
    sigset_t        sa_mask;    /* additional signals to block */
} sigaction_t;

struct queue;
typedef struct __signal_t {
    sigset_t    sig_mask;           // signal set mask.
    sigset_t    sigpending;         /**< Set of pending signals: this is a sticky set a signal is only reset if all pending instances are delivered. */
    queue_t     sig_queue[NSIG];    // queues of siginfo_t * pointers for each signal instance.
    sigaction_t sig_action[NSIG];   // signal action entry for each signal.
    spinlock_t  sig_lock;           // spinlock to protect this struct.
} signal_t;

#define signal_assert(desc)            ({ assert(desc, "No signal description."); })
#define signal_lock(desc)              ({ signal_assert(desc); spin_lock(&(desc)->sig_lock); })
#define signal_unlock(desc)            ({ signal_assert(desc); spin_unlock(&(desc)->sig_lock); })
#define signal_islocked(desc)          ({ signal_assert(desc); spin_islocked(&(desc)->sig_lock); })
#define signal_assert_locked(desc)     ({ signal_assert(desc); spin_assert_locked(&(desc)->sig_lock); })

/**     THREAD SPECIFIC SIGNAL HANDLING FUNCTIONS */

extern int  pause(void);
extern uint alarm(unsigned sec);
extern int  sigpending(sigset_t *set);
extern int  kill(pid_t pid, int signo);
extern int  sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);
extern int  sigwait(const sigset_t *restrict set, int *restrict signop);
extern int  sigaction(int signo, const sigaction_t *restrict act, sigaction_t *restrict oact);

/**     THREAD SPECIFIC SIGNAL HANDLING FUNCTIONS */

extern int  pthread_kill(tid_t thread, int signo);
extern int  pthread_sigqueue(tid_t tid, int signo, union sigval sigval);
extern int  pthread_sigmask(int how, const sigset_t *restrict set, sigset_t *restrict oset);

/**     HELPER FUNCTIONS */

static inline int sig_default_action(int signo) {
    return sig_defaults[signo - 1];
}

static inline bool sig_ignore(int signo) {
    return sig_default_action(signo) == SIG_IGNORE;
}


static inline bool sig_abort(int signo) {
    return sig_default_action(signo) == SIG_ABRT;
}

static inline bool sig_terminate(int signo) {
    return sig_default_action(signo) == SIG_TERM;
}

static inline bool sig_term_dumpcore(int signo) {
    return sig_default_action(signo) == SIG_TERM_CORE;
}

static inline bool sig_stop(int signo) {
    return sig_default_action(signo) == SIG_STOP;
}

static inline bool sig_continue(int signo) {
    return sig_default_action(signo) == SIG_CONT;
}

extern __sighandler_t sig_handler(thread_t *thread, int signo);

static inline bool sig_handler_ignored(__sighandler_t handler, int signo) {
    return handler == SIG_IGN ||
           (handler == SIG_DFL && sig_ignore(signo));
}

/**
 * @brief Retrieve the signal handler for a given thread.
 *
 * @param thread The thread whose signal handler is being requested.
 * @param signo The signal number.
 * @return The signal handler function pointer.
 */
extern __sighandler_t sig_handler(thread_t *thread, int signo);

/**
 * @brief Allocates memory for a new signal descriptor.
 *
 * @param psp Pointer to store the allocated signal descriptor.
 * @return 0 on success, negative error code on failure.
 */
extern int signal_alloc(signal_t **psp);

/**
 * @brief Frees the memory allocated for a signal descriptor.
 *
 * @param sigdesc Pointer to the signal descriptor to free.
 */
extern void signal_free(signal_t *sigdesc);

/**
 * @brief Frees memory allocated for a signal information structure.
 *
 * @param siginfo Pointer to the signal information structure.
 */
extern void siginfo_free(siginfo_t *siginfo);

/**
 * @brief Initializes a siginfo structure with signal details.
 *
 * @param siginfo Pointer to the siginfo structure to initialize.
 * @param signo The signal number.
 * @param val Signal value (additional data).
 * @return 0 on success, -EINVAL if the signal number is invalid.
 */
extern int siginfo_init(siginfo_t *siginfo, int signo, union sigval val);

/**
 * @brief Allocates memory for a siginfo structure.
 *
 * @param psiginfo Pointer to store the allocated siginfo structure.
 * @return 0 on success, negative error code on failure.
 */
extern int siginfo_alloc(siginfo_t **psiginfo);

/**
 * @brief Prints the contents of a siginfo structure.
 *
 * @param siginfo Pointer to the siginfo structure.
 */
extern void siginfo_dump(siginfo_t *siginfo);

/**
 * @brief Enqueues a signal in the process's pending signal queue.
 *
 * @param sigdesc The signal descriptor of the target process.
 * @param siginfo The signal information structure.
 * @return 0 on success, negative error code on failure.
 */
extern int signal_enqueue(signal_t *sigdesc, siginfo_t *siginfo);

/**
 * @brief Dequeues the next pending signal from the process's signal queue.
 *
 * @param sigdesc The signal descriptor of the target process.
 * @param psiginfo Pointer to store the dequeued signal information.
 * @return 0 on success, -ENOENT if no signals are pending.
 */
extern int signal_dequeue(thread_t *thread, siginfo_t **psiginfo);

/**
 * @brief Enqueues a signal into a queue.
 *
 * @param sigqueue The queue to enqueue the signal into.
 * @param siginfo The signal information to enqueue.
 * @return 0 on success, negative error code on failure.
 */
extern int sigqueue_enqueue(queue_t *sigqueue, siginfo_t *siginfo);

/**
 * @brief Dequeues a signal from a queue.
 *
 * @param sigqueue The queue to dequeue from.
 * @param psiginfo Pointer to store the dequeued signal information.
 * @return 0 on success, negative error code on failure.
 */
extern int sigqueue_dequeue(queue_t *sigqueue, siginfo_t **psiginfo);

/**
 * @brief Flushes all signals from a signal queue.
 *
 * @param sigqueue The queue to flush.
 */
extern void sigqueue_flush(queue_t *sigqueue);

extern int  signal_getpending(thread_t *thread, siginfo_t **psiginfo);

extern void  signal_dispatch(void);

extern int  sigmask(sigset_t *sigset, int how, const sigset_t *restrict set, sigset_t *restrict oset);

extern int  thread_sigsend(thread_t *thread, siginfo_t *siginfo);
extern int  thread_kill(thread_t *thread, int signo, union sigval value);
extern int  thread_sigmask(thread_t *thread, int how, const sigset_t *restrict set, sigset_t *restrict oset);
