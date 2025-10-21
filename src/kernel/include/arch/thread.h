#pragma once

#include <arch/ucontext.h>
#include <arch/x86_64/thread.h>
#include <sys/_signal.h>

extern const ulong ARCH_NSIG_NESTED;   // Maximum allowed nested signals per-thread.

/**
 * Arch-specific thread structure.
 * This is what the architecture understands a thread to be.
 * i.e, thread_t is understood by the multitasking concept in ginger
 * but arch_thread_t is what it really is underneath.
 *
 * NOTE: t_contextand t_ucontext;
 * t_context: is what is used by the system to switch contexts
 * by calling swtch() implicitly through sched() or in schedule().
 * see sys/sched/sched.c
 * t_ucontext: is the execution context at the time of
 * interrupts, exceptions, and system calls.
 */
typedef struct arch_thread_t {
    thread_t    *t_thread;      // pointer to thread control block.
    context_t   *t_context;     // caller-callee context.
    ucontext_t  *t_uctx;        // execution context status.
    void        *t_rsvd;        // reserved space on kstack, incase of interrupt chaining.
    usize       t_nsig_nested;  // Nested signals depth.
    flags64_t   t_flags;        // flags.
    uc_stack_t  t_sstack;       // scratch stack for when executing for the first time.
    uc_stack_t  t_kstack;       // kernel stack description.
    uc_stack_t  t_ustack;       // user stack description.
    uc_stack_t  t_altstack;     // if SA_ONSTACK is set for a signal handler, use this stack.
} arch_thread_t;

/**
 * \brief exit execution of thread.
 * \param status exit code from thread.
 */
extern void arch_thread_exit(uintptr_t status);

/**
 * \brief Initialize architecture-specific thread data.
 * \param thread arch-specific thread struct. \todo change to (void *)
 * \param entry thread entry point.
 * \param arg argument to be passed to the thread.
 * \return 0 if successful and errno on failure.
 */
extern int arch_kthread_init(arch_thread_t *thread, thread_entry_t entry, void *arg);

extern int arch_thread_init(arch_thread_t *thread, thread_entry_t entry, void *a0, void *a1, void *a2, void *a3);

extern int x86_64_signal_dispatch(arch_thread_t *thread, sigaction_t *sigact, siginfo_t *info);

extern int arch_thread_execve(arch_thread_t *thread, thread_entry_t entry, int argc, char *const argp[], char *const envp[]);

extern int arch_thread_setkstack(arch_thread_t *arch);
extern int arch_thread_fork(arch_thread_t *dst, arch_thread_t *src);

extern void arch_signal_return(void);
extern int arch_signal_dispatch(arch_thread_t *thread, sigaction_t *sigact, siginfo_t *info);

extern void arch_thread_free(arch_thread_t *arch);