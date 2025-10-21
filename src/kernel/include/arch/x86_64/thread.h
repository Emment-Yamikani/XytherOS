#pragma once

#include <core/types.h>

extern void x86_64_thread_exit(uintptr_t status);
extern int  x86_64_kthread_init(arch_thread_t *arch, thread_entry_t entry, void *arg);
extern int  x86_64_thread_init(arch_thread_t *arch, thread_entry_t entry, void *a0, void *a1, void *a2, void *a3);
extern int  x86_64_thread_execve(arch_thread_t *arch, thread_entry_t entry, int argc, char *const argp[], char *const envp[]);
extern int  x86_64_thread_setkstack(arch_thread_t *arch);
extern int  x86_64_thread_fork(arch_thread_t *dst, arch_thread_t *src);
extern void x86_64_signal_return(void);

extern void x86_64_thread_free(arch_thread_t *arch);