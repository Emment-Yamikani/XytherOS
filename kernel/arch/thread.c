#include <arch/thread.h>
#include <arch/paging.h>
#include <bits/errno.h>
#include <string.h>
#include <mm/kalloc.h>
#include <mm/mmap.h>
#include <sys/_signal.h>
#include <sys/thread.h>

void arch_thread_exit(uintptr_t exit_code) {
#if defined (__x86_64__)
    x86_64_thread_exit(exit_code);
#endif
}

int arch_uthread_init(arch_thread_t *arch, thread_entry_t entry, void *arg) {
#if defined __x86_64__
    return x86_64_uthread_init(arch, entry, arg);
#endif
}

void arch_thread_free(arch_thread_t *arch) {
#if defined __x86_64__
    x86_64_thread_free(arch);
#endif
}

int arch_kthread_init(arch_thread_t *arch, thread_entry_t entry, void *arg) {
#if defined __x86_64__
    return x86_64_kthread_init(arch, entry, arg);
#endif
}

int arch_thread_execve(arch_thread_t *arch, thread_entry_t entry, int argc, const char *argp[], const char *envp[]) {
#if defined __x86_64__
    return x86_64_thread_execve(arch, entry, argc, argp, envp);
#endif
}

int arch_thread_setkstack(arch_thread_t *arch) {
#if defined __x86_64__
    return x86_64_thread_setkstack(arch);
#endif
}

int arch_thread_fork(arch_thread_t *dst, arch_thread_t *src) {
#if defined __x86_64__
    return x86_64_thread_fork(dst, src);
#endif
}

void arch_signal_return(void) {
#if defined (__x86_64__)
    x86_64_signal_return();
#endif
}

int arch_signal_dispatch(arch_thread_t *thread, thread_entry_t entry, siginfo_t *info, sigaction_t *sigact) {
#if defined (__x86_64__)
    return x86_64_signal_dispatch( thread, entry, info, sigact);
#endif
}