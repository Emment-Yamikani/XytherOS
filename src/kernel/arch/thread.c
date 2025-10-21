#include <arch/thread.h>
#include <arch/paging.h>
#include <bits/errno.h>
#include <string.h>
#include <mm/kalloc.h>
#include <mm/mmap.h>
#include <sys/_signal.h>
#include <sys/thread.h>

void arch_thread_exit(uintptr_t status) {
#if defined (__x86_64__)
    x86_64_thread_exit(status);
#endif
}

int arch_thread_init(arch_thread_t *arch, thread_entry_t entry, void *a0, void *a1, void *a2, void *a3) {
#if defined __x86_64__
    return x86_64_thread_init(arch, entry, a0, a1, a2, a3);
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

int arch_thread_execve(arch_thread_t *arch, thread_entry_t entry, int argc, char *const argp[], char *const envp[]) {
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

int arch_signal_dispatch(arch_thread_t *thread, sigaction_t *sigact, siginfo_t *info) {
#if defined (__x86_64__)
    return x86_64_signal_dispatch(thread, sigact, info);
#endif
}