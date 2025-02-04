#pragma once

#include <arch/cpu.h>
#include <arch/thread.h>
#include <core/types.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <mm/mmap.h>
#include <sync/cond.h>
#include <sync/spinlock.h>

typedef enum tstate_t {
    T_EMBRYO    = 0, // Embryo.
    T_READY     = 1, // Ready.
    T_RUNNING   = 2, // Running.
    T_SLEEP     = 3, // Interruptable sleep.
    T_STOPPED   = 4, // Stopped.
    T_ZOMBIE    = 6, // Zombie.
    T_TERMINATED= 6, // Terminated.
} tstate_t;

extern const char *t_states[];
extern char *tget_state(const tstate_t st);

typedef struct proc proc_t;

typedef struct thread_attr_t {
    int         detachstate;
    usize       guardsz;
    uintptr_t   stackaddr;
    usize       stacksz;
} thread_attr_t;

typedef struct thread_sched_t {
    time_t      ts_ctime;       // Thread ceation time.
    time_t      ts_cpu_time;    // CPU time in jiffies(n seconds = (jiffy * (HZ_TO_ns(SYS_HZ) / seconds_TO_ns(1))) ).
    time_t      ts_timeslice;   // Quantum of CPU time for which this thread is allowed to run.
    time_t      ts_total_time;  // Total time this thread has run.
    time_t      ts_last_sched;  // Last time this thread was scheduled to run.
    cpu_t       *ts_processor;  // Current Processor for which this thread has affinity.
    struct {
        enum {
            SOFT_AFFINITY = 0,  /*soft affinity for the cpu*/
            HARD_AFFINITY = 1,  /*hard affinity for the cpu*/
        } type;                 // Type of affinity (SOFT or HARD).
        flags32_t   cpu_set;    // cpu set for which thread can have affinity for.
    } ts_affinity;
    atomic_t    ts_priority;            // Thread scheduling Priority.
} thread_sched_t;

#define sched_DEFAULT() (thread_sched_t){0}

typedef struct sleep_attr_t {
    queue_node_t    *node;              // thread's sleep node.
    queue_t         *queue;             // thread's sleep queue.
    spinlock_t      *guard;             // non-null if sleep queue is associated with a guard lock.
} sleep_attr_t;

typedef struct {
    tid_t           ti_tid;
    tid_t           ti_ktid;
    thread_entry_t  ti_entry;
    tstate_t        ti_state;
    thread_sched_t  ti_sched;
    uintptr_t       ti_errno;
    uintptr_t       ti_exit_code;
    atomic_t        ti_flags;
} thread_info_t;

typedef struct thread_t {
    arch_thread_t   t_arch;
    thread_info_t   t_info;
    sleep_attr_t    t_sleep;
    cond_t          t_event;
    sigset_t        t_sigmask;
    queue_t         t_sigqueue[NSIG];
    queue_t         t_queues;
    spinlock_t      t_lock;
    
    /* The fields below are shared with other threads*/
    pid_t           t_pid;
    cred_t          *t_cred;
    file_ctx_t      *t_fctx;
    mmap_t          *t_mmap;
    queue_t         *t_group;
    sig_desc_t      *t_sigdesc;
} thread_t;

#define THREAD_USER                 BS(0)   // thread is a user thread.
#define THREAD_KILLED               BS(1)   // thread was killed by another thread.
#define THREAD_PARK                 BS(2)   // thread has the park flag set.
#define THREAD_WAKE                 BS(3)   // thread has the wakeup flag set.
#define THREAD_HANDLING_SIG         BS(4)   // thread is currently handling a signal.
#define THREAD_DETACHED             BS(5)   // free resources allocated to this thread imediately to terminates.
#define THREAD_STOP                 BS(6)   // thread stop.
#define THREAD_SIMD_DIRTY           BS(7)   // thread's SIMD context is dirty and must be save on context swtich.
#define THREAD_MAIN                 BS(8)   // thread is a main thread in the tgroup.
#define THREAD_LAST                 BS(9)   // thread is the last thread in the troup.
#define THREAD_CONTINUE             BS(10)  // thread has been continued from a stopped state.
#define THREAD_USING_SSE            BS(11)  // thread is using SSE extensions if this flags is set, FPU otherwise.
#define THREAD_EXITING              BS(12)  // thread is exiting.
#define THREAD_SUSPEND              BS(13)  // flag to suspend thread execution.
#define THREAD_KILLEXCEPT           BS(14)  //
#define THREAD_LOCK_GROUP           BS(15)  // prior to locking thread lock tgroup first.


#define thread_assert(t)            assert(t, "Invalid thread.\n")
#define thread_lock(t)              ({ thread_assert(t); spin_lock(&(t)->t_lock); })
#define thread_unlock(t)            ({ thread_assert(t); spin_unlock(&(t)->t_lock); })
#define thread_islocked(t)          ({ thread_assert(t); spin_islocked(&(t)->t_lock); })
#define thread_test_and_lock(t)     ({ thread_assert(t); spin_test_and_lock(&(t)->t_lock); })
#define thread_assert_locked(t)     ({ thread_assert(t); spin_assert_locked(&(t)->t_lock); })

#define current_assert()            thread_assert(current)
#define current_lock()              thread_lock(current)
#define current_unlock()            thread_unlock(current)
#define current_islocked()          thread_islocked(current)
#define current_assert_locked()     thread_assert_locked(current)

#define thread_test(t, f) ({                      \
    bool locked = thread_test_and_lock(t);        \
    flags64_t flags = (t)->t_info.ti_flags & (f); \
    if (locked)                                   \
        thread_unlock(t);                         \
    flags;                                        \
})

#define thread_set(t, f) ({                \
    bool locked = thread_test_and_lock(t); \
    (t)->t_info.ti_flags |= (f);           \
    if (locked)                            \
        thread_unlock(t);                  \
})

#define thread_mask(t, f) ({               \
    bool locked = thread_test_and_lock(t); \
    (t)->t_info.ti_flags &= ~(f);          \
    if (locked)                            \
        thread_unlock(t);                  \
})

#define thread_ismain(t)            thread_test(t, THREAD_MAIN)
#define thread_islast(t)            thread_test(t, THREAD_LAST)
#define thread_isuser(t)            thread_test(t, THREAD_USER)
#define thread_ispark(t)            thread_test(t, THREAD_PARK)
#define thread_iswake(t)            thread_test(t, THREAD_WAKE)
#define thread_isdetatched(t)       thread_test(t, THREAD_DETACHED)
#define thread_issigctx(t)          thread_test(t, THREAD_HANDLING_SIG)
#define thread_iskill(t)            thread_test(t, THREAD_KILLED)

#define thread_setmain(t)           thread_set(t, THREAD_MAIN)
#define thread_setlast(t)           thread_set(t, THREAD_LAST)
#define thread_setuser(t)           thread_set(t, THREAD_USER)
#define thread_setpark(t)           thread_set(t, THREAD_PARK)
#define thread_setwake(t)           thread_set(t, THREAD_WAKE)
#define thread_setdetatched(t)      thread_set(t, THREAD_DETACHED)
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
#define current_isdetatched()       thread_isdetatched(current)
#define current_issigctx()          thread_issigctx(current)
#define current_iskill()            thread_iskill(current)

#define current_setmain()           thread_setmain(current)
#define current_setlast()           thread_setlast(current)
#define current_setuser()           thread_setuser(current)
#define current_setpark()           thread_setpark(current)
#define current_setwake()           thread_setwake(current)
#define current_setdetathed(t)      thread_setdetatched(current)
#define current_setsigct(t)         thread_setsigctx(current)
#define current_setkill()           thread_setkill(current)

#define current_mask_park(t)        thread_mask_park(current)
#define current_mask_wake(t)        thread_mask_wake(current)
#define current_mask_sigctx(t)      thread_mask_sigctx(current)

extern tid_t gettid(void);
extern void thread_exit(uintptr_t exit_code);


extern pid_t getpid(void);
extern pid_t fork(void);
extern void exit(int exit_code);
extern pid_t getpid(void);
extern pid_t getppid(void);
extern pid_t getsid(pid_t pid);
extern int setsid(void);
extern pid_t getpgrp(void);
extern int setpgrp(void);
extern int getpgid(pid_t pid);
extern pid_t setpgid(pid_t pid, pid_t pgid);
extern int execve(const char *pathname, char *const argv[], char *const envp[]);