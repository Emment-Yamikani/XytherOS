#include <bits/errno.h>
#include <lib/printk.h>
#include <sys/_syscall.h>
#include <arch/ucontext.h>

size_t (*syscall[])() = {
    [SYS_kputc]             = (void *)sys_kputc,
        
    /* File System syscalls */
        
    [SYS_close]             = (void *)sys_close,
    [SYS_unlink]            = (void *)sys_unlink,
    [SYS_dup]               = (void *)sys_dup,
    [SYS_dup2]              = (void *)sys_dup2,
    [SYS_truncate]          = (void *)sys_truncate,
    [SYS_fcntl]             = (void *)sys_fcntl,
    [SYS_ioctl]             = (void *)sys_ioctl,
    [SYS_lseek]             = (void *)sys_lseek,
    [SYS_read]              = (void *)sys_read,
    [SYS_write]             = (void *)sys_write,
    [SYS_open]              = (void *)sys_open,
    [SYS_create]            = (void *)sys_create,
    [SYS_mkdirat]           = (void *)sys_mkdirat,
    [SYS_readdir]           = (void *)sys_readdir,
    [SYS_linkat]            = (void *)sys_linkat,
    [SYS_mknodat]           = (void *)sys_mknodat,
    [SYS_sync]              = (void *)sys_sync,
    [SYS_mount]             = (void *)sys_mount,
    [SYS_chmod]             = (void *)sys_chmod,
    [SYS_rename]            = (void *)sys_rename,
    [SYS_chroot]            = (void *)sys_chroot,
    [SYS_utimes]            = (void *)sys_utimes,
    [SYS_pipe]              = (void *)sys_pipe,
    [SYS_fstat]             = (void *)sys_fstat,
    [SYS_stat]              = (void *)sys_stat,
    [SYS_lstat]             = (void *)sys_lstat,
    [SYS_fstatat]           = (void *)sys_fstatat,
    [SYS_chown]             = (void *)sys_chown,
    [SYS_fchown]            = (void *)sys_fchown,
    [SYS_umask]             = (void *)sys_umask,
    [SYS_isatty]            = (void *)sys_isatty,
    [SYS_mkdir]             = (void *)sys_mkdir,
    [SYS_mknod]             = (void *)sys_mknod,
    [SYS_getcwd]            = (void *)sys_getcwd,
    [SYS_chdir]             = (void *)sys_chdir,
    [SYS_openat]            = (void *)sys_openat,
    [SYS_poll]              = (void *)sys_poll,
        
    /* Process control syscalls */
        
    [SYS_getsid]            = (void *)sys_getsid,
    [SYS_setsid]            = (void *)sys_setsid,
    [SYS_getpgrp]           = (void *)sys_getpgrp,
    [SYS_setpgrp]           = (void *)sys_setpgrp,
    [SYS_getpgid]           = (void *)sys_getpgid,
    [SYS_setpgid]           = (void *)sys_setpgid,
        
    /* Process management syscalls */
        
    [SYS_exit]              = (void *)sys_exit,
    [SYS_getpid]            = (void *)sys_getpid,
    [SYS_getppid]           = (void *)sys_getppid,
    [SYS_fork]              = (void *)sys_fork,
    // [SYS_waitpid]           = (void *)sys_waitpid,
    // [SYS_ptrace]            = (void *)sys_ptrace,
    // [SYS_execve]            = (void *)sys_execve,
    // [SYS_wait4]             = (void *)sys_wait4,
        
    /* Thread management syscalls */
        
    [SYS_gettid]            = (void *)sys_gettid,
    [SYS_thread_join]       = (void *)sys_thread_join,
    [SYS_thread_exit]       = (void *)sys_thread_exit,
    [SYS_thread_create]     = (void *)sys_thread_create,
    [SYS_thread_self]       = (void *)sys_thread_self,
    [SYS_thread_yield]      = (void *)sys_thread_yield,
    // [SYS_park]              = (void *)sys_park,
    // [SYS_unpark]            = (void *)sys_unpark,
    // [SYS_set_thread_area]   = (void *)sys_set_thread_area,
    // [SYS_get_thread_area]   = (void *)sys_get_thread_area,
        
    /* Signal Management syscalls */
        
    [SYS_sigreturn]         = (void *)sys_sigreturn,
    [SYS_pause]             = (void *)sys_pause,
    [SYS_alarm]             = (void *)sys_alarm,
    [SYS_sigpending]        = (void *)sys_sigpending,
    [SYS_kill]              = (void *)sys_kill,
    [SYS_sigaction]         = (void *)sys_sigaction,
    [SYS_sigaltstack]       = (void *)sys_sigaltstack,
    [SYS_sigprocmask]       = (void *)sys_sigprocmask,
    [SYS_sigsuspend]        = (void *)sys_sigsuspend,
    [SYS_sigwaitinfo]       = (void *)sys_sigwaitinfo,
    [SYS_sigtimedwait]      = (void *)sys_sigtimedwait,
        
    /**     THREAD SPECIFIC SIGNAL HANDLING FUNCTIONS */
        
    [SYS_pthread_kill]      = (void *)sys_pthread_kill,
    [SYS_pthread_sigmask]   = (void *)sys_pthread_sigmask,
    [SYS_pthread_sigqueue]  = (void *)sys_pthread_sigqueue,
        
    /** Memory management syscalls */
        
    // [SYS_getpagesize]       = (void *)sys_getpagesize,
    // [SYS_getmemusage]       = (void *)sys_getmemusage,
    // [SYS_mmap]              = (void *)sys_mmap,
    // [SYS_munmap]            = (void *)sys_munmap,
    // [SYS_mprotect]          = (void *)sys_mprotect,
    // [SYS_mlock]             = (void *)sys_mlock,
    // [SYS_mlock2]            = (void *)sys_mlock2,
    // [SYS_munlock]           = (void *)sys_munlock,
    // [SYS_mlockall]          = (void *)sys_mlockall,
    // [SYS_munlockall]        = (void *)sys_munlockall,
    // [SYS_msync]             = (void *)sys_msync,
    // [SYS_mremap]            = (void *)sys_mremap,
    // [SYS_madvise]           = (void *)sys_madvise,
    // [SYS_sbrk]              = (void *)sys_sbrk,
        
    /** Miscelleneous syscalls */
        
    [SYS_uname]             = (void *)sys_uname,
        
    /** Security and User ID syscalls */
        
    [SYS_getuid]            = (void *)sys_getuid,
    [SYS_getgid]            = (void *)sys_getgid,
    [SYS_geteuid]           = (void *)sys_geteuid,
    [SYS_getegid]           = (void *)sys_getegid,
    [SYS_setuid]            = (void *)sys_setuid,
    [SYS_setgid]            = (void *)sys_setgid,
    [SYS_seteuid]           = (void *)sys_seteuid,
    [SYS_setegid]           = (void *)sys_setegid,
        
    /** Network syscalls */
        
    // [SYS_connect]           = (void *)sys_connect,
    // [SYS_accept]            = (void *)sys_accept,
    // [SYS_send]              = (void *)sys_send,
    // [SYS_sendmsg]           = (void *)sys_sendmsg,
    // [SYS_sendto]            = (void *)sys_sendto,
    // [SYS_recv]              = (void *)sys_recv,
    // [SYS_recvmsg]           = (void *)sys_recvmsg,
    // [SYS_recvfrom]          = (void *)sys_recvfrom,
    // [SYS_socket]            = (void *)sys_socket,
    // [SYS_bind]              = (void *)sys_bind,
    // [SYS_getsockname]       = (void *)sys_getsockname,
    // [SYS_listen]            = (void *)sys_listen,
    // [SYS_shutdown]          = (void *)sys_shutdown,
    // [SYS_getpeername]       = (void *)sys_getpeername,
    // [SYS_getsockopt]        = (void *)sys_getsockopt,
        
    /** Time and Clocks syscalls */
        
    [SYS_nanosleep]            = (void *)sys_nanosleep,
    // [SYS_gettimeofday]      = (void *)sys_gettimeofday,
    // [SYS_settimeofday]      = (void *)sys_settimeofday,
    // [SYS_clock_gettime]     = (void *)sys_clock_gettime,
    // [SYS_clock_settime]     = (void *)sys_clock_settime,
    // [SYS_clock_getres]      = (void *)sys_clock_getres,
        
    /** Resource handling syscalls */
        
    // [SYS_setrlimit]         = (void *)sys_setrlimit,
    // [SYS_getrlimit]         = (void *)sys_getrlimit,
    // [SYS_getrusage]         = (void *)sys_getrusage,

};

static int sys_syscall_ni(ucontext_t *uctx) {
    mcontext_t *mctx = &uctx->uc_mcontext;

    printk("syscall(%d) not implemented\n", mctx->rax);
    return -ENOSYS;
}

void do_syscall(ucontext_t *uctx) {
    mcontext_t  *mctx = &uctx->uc_mcontext;

    if (uctx == NULL) {
        return;
    }
    
    if (mctx->rax >= NELEM(syscall)) {
        mctx->rax = sys_syscall_ni(uctx);
    } else if ((long)mctx->rax < 0 || !syscall[mctx->rax]) {
        mctx->rax = sys_syscall_ni(uctx);
    } else {
        pushcli();
        mctx->rax = (syscall[mctx->rax])(mctx->rdi, mctx->rsi, mctx->rdx, mctx->rcx, mctx->r8, mctx->r9);
        popcli();
    }

}