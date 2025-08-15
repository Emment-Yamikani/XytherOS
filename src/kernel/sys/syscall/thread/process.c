#include <bits/errno.h>
#include <core/misc.h>
#include <core/debug.h>
#include <fs/cred.h>
#include <mm/kalloc.h>
#include <sys/thread.h>
#include <sys/sysproc.h>
#include <sys/sysprot.h>
#include <sys/_syscall.h>

/* Process management syscalls */

void sys_exit(int status) {
    exit(status);
}

pid_t sys_getpid(void) {
    return getpid();
}

pid_t sys_getppid(void) {
    return getppid();
}


pid_t sys_fork(void) {
    return fork();
}

pid_t sys_waitpid(pid_t __pid, int *__stat_loc, int __options);
long sys_ptrace(enum __ptrace_request op, pid_t pid, void *addr, void *data);
int sys_execve(const char *pathname, char *const argv[], char *const envp[]);
pid_t sys_wait4(pid_t pid, int *wstatus, int options, void /*struct rusage*/ *rusage);