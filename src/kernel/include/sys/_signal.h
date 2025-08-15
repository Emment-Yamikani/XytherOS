#pragma once

#include <ds/queue.h>
#include <string.h>
#include <sys/sigset.h>
#include <sys/sigaction.h>
#include <sys/siginfo.h>
#include <sync/spinlock.h>

// 0973633531 Helen Kilepa.

#define SIG_ERR     ((sighandler_t) -1)  /* error setting signal disposition. */
#define SIG_DFL     ((sighandler_t) 0)   /* default action taken. */
#define SIG_IGN     ((sighandler_t) 1)   /* ignore this signal. */

/** DEFAULT ACTION FOR SINGNAL(n) */

#define SIG_IGNORE      (1)
#define SIG_ABRT        (2)
#define SIG_TERM        (3)
#define SIG_TERM_CORE   (4)
#define SIG_STOP        (5)
#define SIG_CONT        (6)

extern const char *signal_str[];

extern const int sig_defaults[];

#define SIG_BLOCK   (1)
#define SIG_UNBLOCK (2)
#define SIG_SETMASK (3)

struct queue;
typedef struct __signal_t {
    sigset_t    sig_mask;           /**< signal set mask. */
    /**
     * @brief Set of pending signals.
     * this is a sigset of sticky bits that is only reset,
     * if all pending instances of a particular signal are delivered. */
    sigset_t    sigpending;
    queue_t     sig_queue[NSIG];    /**< queues of siginfo_t * pointers for each signal instance. */
    sigaction_t sig_action[NSIG];   /**< signal action entry for each signal. */
    queue_t     sig_waiters;        /**< signal waiters' queue. */
    spinlock_t  sig_lock;           /**< spinlock to protect this struct. */
} signal_t;

#define signal_assert(desc)            ({ assert(desc, "No signal description.\n"); })
#define signal_lock(desc)              ({ signal_assert(desc); spin_lock(&(desc)->sig_lock); })
#define signal_unlock(desc)            ({ signal_assert(desc); spin_unlock(&(desc)->sig_lock); })
#define signal_islocked(desc)          ({ signal_assert(desc); spin_islocked(&(desc)->sig_lock); })
#define signal_assert_locked(desc)     ({ signal_assert(desc); spin_assert_locked(&(desc)->sig_lock); })

/**     THREAD SPECIFIC SIGNAL HANDLING FUNCTIONS */

extern int  pause(void);
extern uint alarm(unsigned int sec);
extern int  sigpending(sigset_t *set);
extern int  kill(pid_t pid, int signo);
extern int  sigaction(int signo, const sigaction_t *act, sigaction_t *oact);
extern int  sigaltstack(const uc_stack_t *ss, uc_stack_t *oss);
extern int  sigprocmask(int how, const sigset_t *set, sigset_t *oset);
extern int  sigsuspend(const sigset_t *mask);
extern int  sigwaitinfo(const sigset_t *set, siginfo_t *info);
extern int  sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout);

/**     THREAD SPECIFIC SIGNAL HANDLING FUNCTIONS */

extern int  pthread_kill(tid_t thread, int signo);
extern int  pthread_sigmask(int how, const sigset_t *set, sigset_t *oset);
extern int  pthread_sigqueue(tid_t tid, int signo, union sigval sigval);

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

extern sighandler_t sig_handler(thread_t *thread, int signo);

static inline bool sig_handler_ignored(sighandler_t handler, int signo) {
    return handler == SIG_IGN || (handler == SIG_DFL && sig_ignore(signo));
}

/**
 * @brief Retrieve the signal handler for a given thread.
 *
 * @param thread The thread whose signal handler is being requested.
 * @param signo The signal number.
 * @return The signal handler function pointer.
 */
extern sighandler_t sig_handler(thread_t *thread, int signo);

extern int signal_init(signal_t *signals);

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

extern void signal_reset(signal_t *sigdesc);

extern void thread_reset_signal(thread_t *thread);

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

extern int siginfo_create(signo_t signo, union sigval sigval, siginfo_t **psiginfo);

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

extern int signal_desc_send_signal(signal_t *signals, signo_t signo, union sigval sigval);
extern int proc_send_signal(proc_t *proc, signo_t signo, sigval_t sigval);

typedef enum {
    signalTargetThread,
    signalTargetProcess,
} signal_target_t;

/**
 * @brief Dequeues the next pending signal from the process's signal queue.
 *
 * @param sigdesc The signal descriptor of the target process.
 * @param psiginfo Pointer to store the dequeued signal information.
 * @return 0 on success, -ENOENT if no signals are pending.
 */
extern int signal_dequeue(thread_t *thread, sigaction_t *oact, siginfo_t **psiginfo, signal_target_t *ptarget);

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

extern void sigqueue_flush_locked(queue_t *sigqueue);

extern int  signal_getpending(thread_t *thread, siginfo_t **psiginfo);

extern int signal_check_pending(void);
extern int  signal_dispatch(void);

extern int  copy_sigmask(sigset_t *dst, const sigset_t *src);
extern int  sigmask(sigset_t *sigset, int how, const sigset_t *set, sigset_t *oset);

extern int  thread_send_siginfo(thread_t *thread, siginfo_t *siginfo);
extern int  thread_send_signal(thread_t *thread, int signo, union sigval value);
extern int  thread_sigmask(thread_t *thread, int how, const sigset_t *set, sigset_t *oset);
