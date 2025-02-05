#pragma once

#include <arch/cpu.h>
#include <arch/thread.h>
#include <core/types.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <mm/mmap.h>
#include <sync/cond.h>
#include <sync/spinlock.h>

/** @file
 *  @brief Threading library header for xytherOS.
 *  
 *  This header contains the definitions for thread and process structures,
 *  scheduling parameters, synchronization macros, and built-in thread
 *  declarations for xytherOS.
 */

/**
 * @brief Kernel stack size for threads.
 */
#define KSTACK_SIZE     KiB(16)
#define KSTACK_MAXSIZE  KiB(256)

/**
 * @brief User stack size.
 */
#define USTACK_SIZE     KiB(32)
#define USTACK_MAXSIZE  KiB(512)

/*=====================================================================
 *  Thread and Process States
 *====================================================================*/

/**
 * @brief Thread state enumeration.
 *
 * @note T_ZOMBIE and T_TERMINATED are distinct states.
 */
typedef enum tstate_t {
    T_EMBRYO     = 0, /**< Thread is being created. */
    T_READY      = 1, /**< Thread is ready to run. */
    T_RUNNING    = 2, /**< Thread is currently running. */
    T_SLEEP      = 3, /**< Thread is in an interruptible sleep. */
    T_STOPPED    = 4, /**< Thread is stopped. */
    T_ZOMBIE     = 5, /**< Thread has finished execution and awaits resource cleanup. */
    T_TERMINATED = 6  /**< Thread has been terminated. */
} tstate_t;

extern const char *t_states[];
extern char *tget_state(const tstate_t st);

/**
 * @brief Process state enumeration.
 */
typedef enum {
    P_EMBRYO        = 0, /**< Process is being created. */
    P_READY         = 1, /**< Process is ready. */
    P_RUNNING       = 2, /**< Process is running. */
    P_SLEEP         = 3, /**< Process is sleeping. */
    P_STOPPED       = 4, /**< Process is stopped. */
    P_ZOMBIE        = 5, /**< Process has finished execution. */
    P_TERMINATED    = 6  /**< Process has been terminated. */
} proc_state_t;

typedef struct proc proc_t;

/*=====================================================================
 *  Thread Attributes and Scheduling Structures
 *====================================================================*/

/**
 * @brief CPU affinity descriptor.
 *
 * Represents the allowed CPU set for a thread and the type of affinity.
 */
typedef struct cpu_affinity_t {
    /**
     * @brief Affinity type.
     *
     * - SOFT_AFFINITY: Preferred CPU(s) but migration is allowed.
     * - HARD_AFFINITY: Must run on the specified CPU(s).
     */
    enum {
        SOFT_AFFINITY = 0, /**< Soft affinity (preferred CPU(s)). */
        HARD_AFFINITY = 1  /**< Hard affinity (strict CPU binding). */
    } type;
    flags32_t cpu_set; /**< Bitmask representing allowed CPUs. */
} cpu_affinity_t;

/**
 * @brief Thread scheduling information.
 *
 * Contains various timing metrics and affinity settings that influence
 * how the thread is scheduled.
 *
 * All time-related fields are in jiffies.
 */
typedef struct thread_sched_t {
    time_t      ts_ctime;       /**< Thread creation time (jiffies). */
    time_t      ts_cpu_time;    /**< CPU time consumed (jiffies). */
    time_t      ts_timeslice;   /**< Allocated quantum (jiffies). */
    time_t      ts_total_time;  /**< Cumulative run time (jiffies). */
    time_t      ts_last_sched;  /**< Timestamp of last scheduling (jiffies). */

    cpu_t       *ts_processor;  /**< Pointer to the current processor. */
    cpu_affinity_t ts_affin;    /**< CPU affinity information. */

    atomic_t    ts_priority;    /**< Scheduling priority (can be static or dynamic). */

    /* Additional metrics for advanced scheduling could be added here:
     * e.g., total wait time, preemption count, etc.
     */
} thread_sched_t;

/**
 * @brief Thread sleep attributes.
 */
typedef struct sleep_attr_t {
    queue_node_t    *node;  /**< Node for the sleep queue. */
    queue_t         *queue; /**< Associated sleep queue. */
    spinlock_t      *guard; /**< Non-null if the sleep queue is guarded by a lock. */
} sleep_attr_t;

/**
 * @brief Thread attribute structure.
 */
typedef struct thread_attr_t {
    int         detachstate; /**< Detach state. */
    usize       guardsz;     /**< Size of guard region. */
    uintptr_t   stackaddr;   /**< Stack address. */
    usize       stacksz;     /**< Stack size. */
} thread_attr_t;

#define UTHREAD_ATTR_DEFAULT (thread_attr_t){ .detachstate = 0, .stackaddr = 0, .guardsz = PGSZ,   .stacksz = USTACK_SIZE }
#define KTHREAD_ATTR_DEFAULT (thread_attr_t){ .detachstate = 0, .stackaddr = 0, .guardsz = PAGESZ,   .stacksz = KSTACK_SIZE }

/**
 * @brief Thread information structure.
 */
typedef struct {
    tid_t           ti_tid;         /**< Thread's unique ID. */
    tid_t           ti_ktid;        /**< ID of the thread that terminated this thread (if any). */
    tid_t           ti_tgid;        /**< Thread group ID. */
    thread_entry_t  ti_entry;       /**< Thread entry point. */
    tstate_t        ti_state;       /**< Current state of the thread. */
    thread_sched_t  ti_sched;       /**< Scheduling information. */
    uintptr_t       ti_errno;       /**< Per-thread error number. */
    atomic_t        ti_flags;       /**< Thread flags. */
    uintptr_t       ti_exit_code;   /**< Exit code when the thread terminates. */
} thread_info_t;

/**
 * @brief Process information structure.
 */
typedef struct __proc_info_t proc_info_t;
struct __proc_info_t {
    pid_t           pid;            /**< Process ID. */
    pid_t           sid;            /**< Session ID. */
    pid_t           pgid;           /**< Process group ID. */
    proc_info_t     *parent;        /**< Pointer to the parent process. */
    proc_state_t    state;          /**< Process state. */
    unsigned long   flags;          /**< Process flags. */
    long            exit_code;      /**< Process exit status. */
    thread_entry_t  entry;          /**< Process entry point. */
    char            *name;          /**< Process name. */
    cond_t          child_event;    /**< Condition variable for child wait events. */
    queue_t         children;       /**< Queue of child processes. */
    spinlock_t      lock;           /**< Lock to protect this structure. */
};

/*=====================================================================
 *  Thread Structure and Flags
 *====================================================================*/

/**
 * @brief Thread structure.
 *
 * Contains architecture-specific data, scheduling, synchronization,
 * and process association information.
 */
typedef struct thread_t {
    arch_thread_t   t_arch;         /**< Architecture-specific thread context. */
    thread_info_t   t_info;         /**< General thread information. */
    sleep_attr_t    t_sleep;        /**< Sleep-related information. */
    cond_t          t_event;        /**< Condition variable for thread events. */
    queue_t         t_queues;       /**< Queue node used by various thread queues. */
    sigset_t        t_sigmask;      /**< Signal mask for the thread. */
    queue_t         t_sigqueue[NSIG]; /**< Per-thread signal queues. */
    spinlock_t      t_lock;         /**< Lock protecting the thread structure. */

    /* Fields shared with other threads in the same process */
    proc_info_t     *t_proc;        /**< Associated process information. */
    cred_t          *t_cred;        /**< Credentials. */
    file_ctx_t      *t_fctx;        /**< File context. */
    mmap_t          *t_mmap;        /**< Memory mapping for the process. */
    queue_t         *t_group;       /**< Thread group queue. */
    sig_desc_t      *t_signal;      /**< Per-process signal descriptor. */
} __aligned(16) thread_t;

/* Thread creation flags */
#define THREAD_CREATE_USER          BS(0)
#define THREAD_CREATE_GROUP         BS(1)
#define THREAD_CREATE_DETACHED      BS(2)
#define THREAD_CREATE_SCHED         BS(3)

/* Thread state flags */
#define THREAD_USER                 BS(0)   /**< Thread is a user thread. */
#define THREAD_KILLED               BS(1)   /**< Thread has been killed. */
#define THREAD_PARK                 BS(2)   /**< Park flag is set. */
#define THREAD_WAKE                 BS(3)   /**< Wakeup flag is set. */
#define THREAD_HANDLING_SIG         BS(4)   /**< Thread is handling a signal. */
#define THREAD_DETACHED             BS(5)   /**< Thread is detached. */
#define THREAD_STOP                 BS(6)   /**< Stop flag. */
#define THREAD_SIMD_DIRTY           BS(7)   /**< SIMD context is dirty; save on context switch. */
#define THREAD_MAIN                 BS(8)   /**< Main thread of the group. */
#define THREAD_LAST                 BS(9)   /**< Last thread in the group. */
#define THREAD_CONTINUE             BS(10)  /**< Continued from a stopped state. */
#define THREAD_USING_SSE            BS(11)  /**< Thread uses SSE extensions (FPU otherwise). */
#define THREAD_EXITING              BS(12)  /**< Thread is exiting. */
#define THREAD_SUSPEND              BS(13)  /**< Thread execution is suspended. */
#define THREAD_KILLEXCEPT           BS(14)  /**< Special kill exception flag. */
#define THREAD_LOCK_GROUP           BS(15)  /**< Lock thread group before locking thread structure. */

/*=====================================================================
 *  Thread Locking and Flag Manipulation Macros
 *====================================================================*/

/**
 * @brief Assert that a thread pointer is valid.
 */
#define thread_assert(t)            assert(t, "Invalid thread.\n")

/**
 * @brief Acquire a thread's lock.
 */
#define thread_lock(t)              ({ thread_assert(t); spin_lock(&(t)->t_lock); })

/**
 * @brief Release a thread's lock.
 */
#define thread_unlock(t)            ({ thread_assert(t); spin_unlock(&(t)->t_lock); })

/**
 * @brief Check if a thread's lock is held.
 */
#define thread_islocked(t)          ({ thread_assert(t); spin_islocked(&(t)->t_lock); })

/**
 * @brief Test and lock a thread.
 */
#define thread_test_and_lock(t)     ({ thread_assert(t); spin_test_and_lock(&(t)->t_lock); })

/**
 * @brief Assert that a thread is locked.
 */
#define thread_assert_locked(t)     ({ thread_assert(t); spin_assert_locked(&(t)->t_lock); })

/* Macros for the current thread (global variable `current` assumed) */
#define current_assert()            thread_assert(current)
#define current_lock()              thread_lock(current)
#define current_unlock()            thread_unlock(current)
#define current_islocked()          thread_islocked(current)
#define current_assert_locked()     thread_assert_locked(current)

/**
 * @brief Test a thread's flags.
 */
#define thread_test(t, f) ({                      \
    bool locked = thread_test_and_lock(t);        \
    flags64_t flags = (t)->t_info.ti_flags & (f); \
    if (locked)                                   \
        thread_unlock(t);                         \
    flags;                                        \
})

/**
 * @brief Set specific flags on a thread.
 */
#define thread_set(t, f) ({                \
    bool locked = thread_test_and_lock(t); \
    (t)->t_info.ti_flags |= (f);           \
    if (locked)                            \
        thread_unlock(t);                  \
})

/**
 * @brief Clear specific flags on a thread.
 */
#define thread_mask(t, f) ({               \
    bool locked = thread_test_and_lock(t); \
    (t)->t_info.ti_flags &= ~(f);          \
    if (locked)                            \
        thread_unlock(t);                  \
})

/* Convenience flag macros */
#define thread_ismain(t)            thread_test(t, THREAD_MAIN)
#define thread_islast(t)            thread_test(t, THREAD_LAST)
#define thread_isuser(t)            thread_test(t, THREAD_USER)
#define thread_ispark(t)            thread_test(t, THREAD_PARK)
#define thread_iswake(t)            thread_test(t, THREAD_WAKE)
#define thread_isdetached(t)        thread_test(t, THREAD_DETACHED)
#define thread_issigctx(t)          thread_test(t, THREAD_HANDLING_SIG)
#define thread_iskill(t)            thread_test(t, THREAD_KILLED)

#define thread_setmain(t)           thread_set(t, THREAD_MAIN)
#define thread_setlast(t)           thread_set(t, THREAD_LAST)
#define thread_setuser(t)           thread_set(t, THREAD_USER)
#define thread_setpark(t)           thread_set(t, THREAD_PARK)
#define thread_setwake(t)           thread_set(t, THREAD_WAKE)
#define thread_setdetached(t)       thread_set(t, THREAD_DETACHED)
#define thread_setsigctx(t)         thread_set(t, THREAD_HANDLING_SIG)
#define thread_setkill(t)           thread_set(t, THREAD_KILLED)

#define thread_mask_park(t)         thread_mask(t, THREAD_PARK)
#define thread_mask_wake(t)         thread_mask(t, THREAD_WAKE)
#define thread_mask_sigctx(t)       thread_mask(t, THREAD_HANDLING_SIG)

#define current_ismain()            thread_ismain(current)
#define current_islast()            thread_islast(current)
#define current_isuser()            thread_isuser(current)
#define current_ispark()            thread_ispark(current)
#define current_iswake()            thread_iswake(current)
#define current_isdetached()        thread_isdetached(current)
#define current_issigctx()          thread_issigctx(current)
#define current_iskill()            thread_iskill(current)

#define current_setmain()           thread_setmain(current)
#define current_setlast()           thread_setlast(current)
#define current_setuser()           thread_setuser(current)
#define current_setpark()           thread_setpark(current)
#define current_setwake()           thread_setwake(current)
#define current_setdetached()       thread_setdetached(current)
#define current_setsigctx()         thread_setsigctx(current)
#define current_setkill()           thread_setkill(current)

#define current_mask_park()         thread_mask_park(current)
#define current_mask_wake()         thread_mask_wake(current)
#define current_mask_sigctx()       thread_mask_sigctx(current)

/**
 * @brief Safely transition a thread to a new state.
 *
 * Validates the new state before assignment.
 *
 * @param t Thread pointer.
 * @param s New thread state.
 *
 * @return 0 on success, or -EINVAL if the state is invalid.
 */
#define thread_enter_state(t, s) ({             \
    bool locked = thread_test_and_lock(t);      \
    int err = 0;                                \
    if ((s) >= T_EMBRYO && (s) <= T_TERMINATED) \
        (t)->t_info.ti_state = (s);             \
    else                                        \
        err = -EINVAL;                          \
    if (locked)                                 \
        thread_unlock(t);                       \
    err;                                        \
})

#define current_enter_state(s)      thread_enter_state(current, s)

/**
 * @brief Retrieve a thread's current state.
 *
 * @param t Thread pointer.
 *
 * @return The current thread state.
 */
#define thread_get_state(t) ({             \
    bool locked = thread_test_and_lock(t); \
    tstate_t state = (t)->t_info.ti_state; \
    if (locked)                            \
        thread_unlock(t);                  \
    state;                                 \
})

#define current_get_state()         thread_get_state(current)

/* Convenience state-checking macros */
#define thread_isembryo(t)          (thread_get_state(t) == T_EMBRYO)
#define thread_isready(t)           (thread_get_state(t) == T_READY)
#define thread_isrunning(t)         (thread_get_state(t) == T_RUNNING)
#define thread_issleep(t)           (thread_get_state(t) == T_SLEEP)
#define thread_isstopped(t)         (thread_get_state(t) == T_STOPPED)
#define thread_iszombie(t)          (thread_get_state(t) == T_ZOMBIE)
#define thread_isterminated(t)      (thread_get_state(t) == T_TERMINATED)

#define current_isembryo()          thread_isembryo(current)
#define current_isready()           thread_isready(current)
#define current_isrunning()         thread_isrunning(current)
#define current_issleep()           thread_issleep(current)
#define current_isstopped()         thread_isstopped(current)
#define current_iszombie()          thread_iszombie(current)
#define current_isterminated()      thread_isterminated(current)

/*=====================================================================
 *  Builtin Threads
 *====================================================================*/

/**
 * @brief Builtin thread descriptor.
 *
 * Used to declare built-in threads that are automatically discovered.
 */
typedef struct {
    char    *thread_name;  /**< Name of the thread. */
    void    *thread_arg;   /**< Argument to pass to the thread. */
    void    *thread_entry; /**< Entry point function. */
    void    *thread_rsvd;  /**< Reserved for future use. */
} builtin_thread_t;

extern builtin_thread_t __builtin_thrds[];
extern builtin_thread_t __builtin_thrds_end[];

/**
 * @brief Declare a builtin thread.
 *
 * This macro places the thread descriptor in the __builtin_thrds section.
 *
 * @param name  Identifier name for the thread.
 * @param entry Entry point for the thread.
 * @param arg   Argument for the thread.
 */
#define BUILTIN_THREAD(name, entry, arg)          \
builtin_thread_t __used_section(.__builtin_thrds) \
    __thread_##name     = {                       \
        .thread_name    = #name,                  \
        .thread_arg     = arg,                    \
        .thread_entry   = entry,                  \
}

/**
 * @brief Announce a builtin thread's startup (for debugging).
 *
 * @param name Name of the thread.
 */
#define BUILTIN_THREAD_ANNOUNCE(name)    ({\
    printk("%s:%d: \"%s\" thread [tid: %d] running...\n",\
    __FILE__, __LINE__, name, thread_self());\
})

/*=====================================================================
 *  Miscellaneous Definitions and Function Prototypes
 *====================================================================*/

/* Thread and process function prototypes */
extern tid_t    gettid(void);

extern tid_t    thread_gettid(thread_t *thread);
extern tid_t    thread_self(void);
extern void     thread_exit(uintptr_t exit_code);
extern int      thread_alloc(usize stack_size, int flags, thread_t **ptp);
extern int      thread_enqueue(queue_t *queue, thread_t *thread, queue_node_t **pnp);
extern thread_t *thread_dequeue(queue_t *queue);
extern int      thread_queue_peek(queue_t *queue, tid_t tid, thread_t **ptp);
extern int      thread_create(thread_attr_t *attr, thread_entry_t entry, void *arg, int flags, thread_t **ptp);

extern pid_t    fork(void);
extern void     exit(int exit_code);
extern pid_t    getpid(void);
extern pid_t    getppid(void);
extern pid_t    getsid(pid_t pid);
extern int      setsid(void);
extern pid_t    getpgrp(void);
extern int      setpgrp(void);
extern int      getpgid(pid_t pid);
extern pid_t    setpgid(pid_t pid, pid_t pgid);
extern int      execve(const char *pathname, char *const argv[], char *const envp[]);
