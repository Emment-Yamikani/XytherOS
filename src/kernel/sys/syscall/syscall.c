#include <bits/errno.h>
#include <core/misc.h>
#include <core/debug.h>
#include <fs/cred.h>
#include <mm/kalloc.h>
#include <sys/thread.h>
#include <sys/sysproc.h>
#include <sys/sysprot.h>
#include <sys/_syscall.h>


void sys_kputc(int c) {
    printk("%c", c);
}

/* File System syscalls */

int sys_close(int fd) {
    return close(fd);
}

int sys_unlink(int fd) {
    return unlink(fd);
}

int sys_dup(int fd) {
    return dup(fd);
}

int sys_dup2(int fd1, int fd2) {
    return dup2(fd1, fd2);
}

int sys_truncate(int fd, off_t length) {
    return truncate(fd, length);
}

int sys_fcntl(int fd, int cmd, void *argp) {
    return fcntl(fd, cmd, argp);
}

int sys_ioctl(int fd, int req, void *argp) {
    return ioctl(fd, req, argp);
}

off_t sys_lseek(int fd, off_t off, int whence) {
    return lseek(fd, off, whence);
}

ssize_t sys_read(int fd, void *buf, size_t size) {
    return read(fd, buf, size);
}

ssize_t sys_write(int fd, const void *buf, size_t size) {
    return write(fd, buf, size);
}

int sys_open(const char *pathname, int oflags, mode_t mode) {
    return open(pathname, oflags, mode);
}

int sys_create(const char *filename, mode_t mode) {
    return create(filename, mode);
}

int sys_mkdirat(int fd, const char *filename, mode_t mode) {
    return mkdirat(fd, filename, mode);
}

ssize_t sys_readdir(int fd, off_t off, void *buf, size_t count) {
    return readdir(fd, off, buf, count);
}

int sys_linkat(int fd, const char *oldname, const char *newname) {
    return linkat(fd, oldname, newname);
}

int sys_mknodat(int fd, const char *filename, mode_t mode, int devid) {
    return mknodat(fd, filename, mode, devid);
}

int sys_sync(int fd) {
    return sync(fd);
}

int sys_mount(const char *source, const char *target, const char *type, unsigned long flags, const void *data) {
    return mount(source, target, type, flags, data);
}

int sys_chmod(const char *pathname __unused, mode_t mode __unused) {
    return -ENOSYS;
}

int sys_rename(const char *oldpath __unused, const char *newpath __unused) {
    return -ENOSYS;
}

int sys_chroot(const char *path __unused) {
    return -ENOSYS;
}

int sys_utimes(const char *filename __unused, const struct timeval times[2] __unused) {
    return -ENOSYS;
}

int sys_pipe(int fds[2]) {
    return pipe(fds);
}

int sys_fstat(int fildes, struct stat *buf) {
    return fstat(fildes, buf);
}

int sys_stat(const char *restrict path, struct stat *restrict buf) {
    return stat(path, buf);
}

int sys_lstat(const char *restrict path, struct stat *restrict buf) {
    return lstat(path, buf);
}

int sys_fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag) {
    return fstatat(fd, path, buf, flag);
}

int sys_chown(const char *pathname, uid_t uid, gid_t gid) {
    return chown(pathname, uid, gid);
}

int sys_fchown(int fd, uid_t uid, gid_t gid) {
    return fchown(fd, uid, gid);
}

mode_t sys_umask(mode_t cmask) {
    return umask(cmask);
}

int sys_isatty(int fd) {
    return isatty(fd);
}

int sys_mkdir(const char *filename, mode_t mode) {
    return mkdir(filename, mode);
}

int sys_mknod(const char *filename, mode_t mode, int devid) {
    return mknod(filename, mode, devid);
}

int sys_getcwd(char *buf, size_t size) {
    return getcwd(buf, size);
}

int sys_chdir(const char *path) {
    return chdir(path);
}

int sys_openat(int fd, const char *pathname, int oflags, mode_t mode) {
    return openat(fd, pathname, oflags, mode);
}

int sys_poll(struct pollfd fds[] __unused, nfds_t nfds __unused, int timeout __unused) {
    return -ENOSYS;
}

/* Process control syscalls */

pid_t sys_getsid(pid_t pid) {
    return getsid(pid);
}

pid_t sys_setsid(void) {
    return setsid();
}

pid_t sys_getpgrp(void) {
    return getpgrp();
}

pid_t sys_setpgrp(void) {
    return setpgrp();
}

pid_t sys_getpgid(pid_t pid) {
    return getpgid(pid);
}

int sys_setpgid(pid_t pid, pid_t pgid) {
    return setpgid(pid, pgid);
}

/* Signal Management syscalls */

void sys_sigreturn() {
    return;
    // sigreturn();
}

int  sys_pause(void) {
    return pause();
}

uint sys_alarm(unsigned int sec) {
    return alarm(sec);
}

int  sys_sigpending(sigset_t *set) {
    return sigpending(set);
}

int  sys_kill(pid_t pid, int signo) {
    return kill(pid, signo);
}

int  sys_sigaction(int signo, const sigaction_t *act, sigaction_t *oact) {
    return sigaction(signo, act, oact);
}

int  sys_sigaltstack(const uc_stack_t *ss, uc_stack_t *oss) {
    return sigaltstack(ss, oss);
}

int  sys_sigprocmask(int how, const sigset_t *set, sigset_t *oset) {
    return sigprocmask(how, set, oset);
}

int  sys_sigsuspend(const sigset_t *mask) {
    return sigsuspend(mask);
}

int  sys_sigwaitinfo(const sigset_t *set, siginfo_t *info) {
    return sigwaitinfo(set, info);
}

int  sys_sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout) {
    return sigtimedwait(set, info, timeout);
}


/**     THREAD SPECIFIC SIGNAL HANDLING FUNCTIONS */

int  sys_pthread_kill(tid_t thread, int signo) {
    return pthread_kill(thread, signo);
}

int  sys_pthread_sigmask(int how, const sigset_t *set, sigset_t *oset) {
    return pthread_sigmask(how, set, oset);
}

int  sys_pthread_sigqueue(tid_t tid, int signo, union sigval sigval) {
    return pthread_sigqueue(tid, signo, sigval);
}


/** Memory management syscalls */

int sys_getpagesize(void);
void sys_getmemusage(meminfo_t *info);
void *sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int sys_munmap(void *addr, size_t len);
int sys_mprotect(void *addr, size_t len, int prot);
int sys_mlock(const void *addr, size_t len);
int sys_mlock2(const void *addr, size_t len, unsigned int flags);
int sys_munlock(const void *addr, size_t len);
int sys_mlockall(int flags);
int sys_munlockall(void);
int sys_msync(void *addr, size_t length, int flags);
void *sys_mremap(void *old_address, size_t old_size, size_t new_size, int flags, ... /* void *new_address */);
int sys_madvise(void *addr, size_t length, int advice);
void *sys_sbrk(intptr_t increment);

/** Miscelleneous syscalls */

int sys_uname(struct utsname *name) {
    return uname(name);
}


/** Security and User ID syscalls */

uid_t sys_getuid(void) {
    return getuid();
}

gid_t sys_getgid(void) {
    return getgid();
}

uid_t sys_geteuid(void) {
    return geteuid();
}

gid_t sys_getegid(void) {
    return getegid();
}

int sys_setuid(uid_t uid) {
    return setuid(uid);
}

int sys_setgid(gid_t gid) {
    return setgid(gid);
}

int sys_seteuid(uid_t euid) {
    return seteuid(euid);
}

int sys_setegid(gid_t egid) {
    return setegid(egid);
}


/** Network syscalls */

int sys_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int sys_accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
isize sys_send(int sockfd, const void *buf, usize len, int flags);
isize sys_sendmsg(int sockfd, const struct msghdr *msg, int flags);
isize sys_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
isize sys_recv(int sockfd, void *buf, usize len, int flags);
isize sys_recvmsg(int sockfd, struct msghdr *msg, int flags);
isize sys_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *restrict src_addr, socklen_t *restrict addrlen);
int sys_socket(int domain, int type, int protocol);
int sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int sys_getsockname(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
int sys_listen(int sockfd, int backlog);
int sys_shutdown(int sockfd, int how);
int sys_getpeername(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
int sys_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *restrict optlen);

/** Time and Clocks syscalls */

int sys_nanosleep(const timespec_t *duration, timespec_t *rem) {
    return nanosleep(duration, rem);
}

int sys_gettimeofday(struct timeval *restrict tp, void *restrict tzp) {
    return gettimeofday(tp, tzp);
}

int sys_settimeofday(const struct timeval *tv, const struct timezone *tz) {
    return settimeofday(tv, tz);
}


int sys_clock_gettime(clockid_t clockid, struct timespec *tp);
int sys_clock_settime(clockid_t clockid, const struct timespec *tp);
int sys_clock_getres(clockid_t clockid, struct timespec *res);

/** Resource handling syscalls */

int sys_setrlimit(int resource, const void /*struct rlimit*/ *rlim);
int sys_getrlimit(int resource, void /*struct rlimit*/ *rlim);
int sys_getrusage(int who, void /*struct rusage*/ *usage);