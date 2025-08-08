#include <xyther/syscall.h>

void kputc(int c) {
    return sys_kputc(c);
}

int close(int fd) {
    return sys_close(fd);
}

int unlink(int fd) {
    return sys_unlink(fd);
}

int dup(int fd) {
    return sys_dup(fd);
}

int dup2(int fd1, int fd2) {
    return sys_dup2(fd1, fd2);
}

int truncate(int fd, off_t length) {
    return sys_truncate(fd, length);
}

int fcntl(int fd, int cmd, void *argp) {
    return sys_fcntl(fd, cmd, argp);
}

int ioctl(int fd, int req, void *argp) {
    return sys_ioctl(fd, req, argp);
}

off_t lseek(int fd, off_t off, int whence) {
    return sys_lseek(fd, off, whence);
}

ssize_t read(int fd, void *buf, size_t size) {
    return sys_read(fd, buf, size);
}

ssize_t write(int fd, const void *buf, size_t size) {
    return sys_write(fd, buf, size);
}

int open(const char *pathname, int oflags, mode_t mode) {
    return sys_open(pathname, oflags, mode);
}

int create(const char *filename, mode_t mode) {
    return sys_create(filename, mode);
}

int mkdirat(int fd, const char *filename, mode_t mode) {
    return sys_mkdirat(fd, filename, mode);
}

ssize_t readdir(int fd, off_t off, void *buf, size_t count) {
    return sys_readdir(fd, off, buf, count);
}

int linkat(int fd, const char *oldname, const char *newname) {
    return sys_linkat(fd, oldname, newname);
}

int mknodat(int fd, const char *filename, mode_t mode, int devid) {
    return sys_mknodat(fd, filename, mode, devid);
}

int sync(int fd) {
    return sys_sync(fd);
}

int mount(const char *source, const char *target, const char *type, unsigned long flags, const void *data) {
    return sys_mount(source, target, type, flags, data);
}

int chmod(const char *pathname, mode_t mode) {
    return sys_chmod(pathname, mode);
}

int rename(const char *oldpath, const char *newpath) {
    return sys_rename(oldpath, newpath);
}

int chroot(const char *path) {
    return sys_chroot(path);
}

int utimes(const char *filename, const struct timeval times[2]) {
    return sys_utimes(filename, times);
}

int pipe(int fds[2]) {
    return sys_pipe(fds);
}

int fstat(int fildes, struct stat *buf) {
    return sys_fstat(fildes, buf);
}

int stat(const char *restrict path, struct stat *restrict buf) {
    return sys_stat(path, buf);
}

int lstat(const char *restrict path, struct stat *restrict buf) {
    return sys_lstat(path, buf);
}

int fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag) {
    return sys_fstatat(fd, path, buf, flag);
}

int chown(const char *pathname, uid_t uid, gid_t gid) {
    return sys_chown(pathname, uid, gid);
}

int fchown(int fd, uid_t uid, gid_t gid) {
    return sys_fchown(fd, uid, gid);
}

mode_t umask(mode_t cmask) {
    return sys_umask(cmask);
}

int isatty(int fd) {
    return sys_isatty(fd);
}

int mkdir(const char *filename, mode_t mode) {
    return sys_mkdir(filename, mode);
}

int mknod(const char *filename, mode_t mode, int devid) {
    return sys_mknod(filename, mode, devid);
}

int getcwd(char *buf, size_t size) {
    return sys_getcwd(buf, size);
}

int chdir(const char *path) {
    return sys_chdir(path);
}

int openat(int fd, const char *pathname, int oflags, mode_t mode) {
    return sys_openat(fd, pathname, oflags, mode);
}

int poll(struct pollfd fds[] __unused, nfds_t nfds __unused, int timeout __unused) {
    return sys_poll(fds, nfds, timeout);
}

pid_t getsid(pid_t pid) {
    return sys_getsid(pid);
}

pid_t setsid(void) {
    return sys_setsid();
}

pid_t getpgrp(void) {
    return sys_getpgrp();
}

pid_t setpgrp(void) {
    return sys_setpgrp();
}

pid_t getpgid(pid_t pid) {
    return sys_getpgid(pid);
}

int setpgid(pid_t pid, pid_t pgid) {
    return sys_setpgid(pid, pgid);
}


pid_t fork(void) {
    return sys_fork();
}

pid_t getpid(void) {
    return sys_getpid();
}

pid_t getppid(void) {
    return sys_getppid();
}

void exit(int exit_code) {
    return sys_exit(exit_code);
}

pid_t waitpid(pid_t __pid, int *__stat_loc, int __options) {
    return sys_waitpid(__pid, __stat_loc, __options);
}

long ptrace(enum __ptrace_request op, pid_t pid, void *addr, void *data) {
    return sys_ptrace(op, pid, addr, data);
}

int execve(const char *pathname, char *const argv[], char *const envp[]) {
    return sys_execve(pathname, argv, envp);
}

pid_t wait4(pid_t pid, int *wstatus, int options, void /*struct rusage*/ *rusage) {
    return sys_wait4(pid, wstatus, options, rusage);
}


int park(void) {
    return sys_park();
}

int unpark(tid_t tid) {
    return sys_unpark(tid);
}

tid_t gettid(void) {
    return sys_gettid();
}

void thread_exit(int exit_code) {
    return sys_thread_exit(exit_code);
}

int thread_create(tid_t *ptidp, void *attr, thread_entry_t entry, void *arg) {
    return sys_thread_create(ptidp, attr, entry, arg);
}

int thread_join(tid_t tid, void **retval) {
    return sys_thread_join(tid, retval);
}

int set_thread_area(void *addr) {
    return sys_set_thread_area(addr);
}

int get_thread_area(void *addr) {
    return sys_get_thread_area(addr);
}

tid_t thread_self(void) {
    return sys_thread_self();
}

void thread_yield(void) {
    return sys_thread_yield();
}


void sigreturn() {
    return sys_sigreturn();
}

int  pause(void) {
    return sys_pause();
}

unsigned alarm(unsigned int sec) {
    return sys_alarm(sec);
}

int  sigpending(sigset_t *set) {
    return sys_sigpending(set);
}

int  kill(pid_t pid, int signo) {
    return sys_kill(pid, signo);
}

int  sigaction(int signo, const sigaction_t *act, sigaction_t *oact) {
    return sys_sigaction(signo, act, oact);
}

int  sigaltstack(const uc_stack_t *ss, uc_stack_t *oss) {
    return sys_sigaltstack(ss, oss);
}

int  sigprocmask(int how, const sigset_t *set, sigset_t *oset) {
    return sys_sigprocmask(how, set, oset);
}

int  sigsuspend(const sigset_t *mask) {
    return sys_sigsuspend(mask);
}

int  sigwaitinfo(const sigset_t *set, siginfo_t *info) {
    return sys_sigwaitinfo(set, info);
}

int  sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout) {
    return sys_sigtimedwait(set, info, timeout);
}


int  pthread_kill(tid_t thread, int signo) {
    return sys_pthread_kill(thread, signo);
}

int  pthread_sigmask(int how, const sigset_t *set, sigset_t *oset) {
    return sys_pthread_sigmask(how, set, oset);
}

int  pthread_sigqueue(tid_t tid, int signo, union sigval sigval) {
    return sys_pthread_sigqueue(tid, signo, sigval);
}

int getpagesize(void) {
    return sys_getpagesize();
}

void getmemusage(meminfo_t *info) {
    return sys_getmemusage(info);
}

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off) {
    return sys_mmap(addr, len, prot, flags, fd, off);
}

int munmap(void *addr, size_t len) {
    return sys_munmap(addr, len);
}

int mprotect(void *addr, size_t len, int prot) {
    return sys_mprotect(addr, len, prot);
}

int mlock(const void *addr, size_t len) {
    return sys_mlock(addr, len);
}

int mlock2(const void *addr, size_t len, unsigned int flags) {
    return sys_mlock2(addr, len, flags);
}

int munlock(const void *addr, size_t len) {
    return sys_munlock(addr, len);
}

int mlockall(int flags) {
    return sys_mlockall(flags);
}

int munlockall(void) {
    return sys_munlockall();
}

int msync(void *addr, size_t length, int flags) {
    return sys_msync(addr, length, flags);
}

void *mremap(void *old_address, size_t old_size, size_t new_size, int flags, ... /* void *new_address */) {
    return sys_mremap(old_address, old_size, new_size, flags/* void *new_address */);
}

int madvise(void *addr, size_t length, int advice) {
    return sys_madvise(addr, length, advice);
}

void *sbrk(intptr_t increment) {
    return sys_sbrk(increment);
}

int uname(struct utsname *name) {
    return sys_uname(name);
}

uid_t getuid(void) {
    return sys_getuid();
}

gid_t getgid(void) {
    return sys_getgid();
}

uid_t geteuid(void) {
    return sys_geteuid();
}

gid_t getegid(void) {
    return sys_getegid();
}

int setuid(uid_t uid) {
    return sys_setuid(uid);
}

int setgid(gid_t gid) {
    return sys_setgid(gid);
}

int seteuid(uid_t euid) {
    return sys_seteuid(euid);
}

int setegid(gid_t egid) {
    return sys_setegid(egid);
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return sys_connect(sockfd, addr, addrlen);
}

int accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen) {
    return sys_accept(sockfd, addr, addrlen);
}

isize send(int sockfd, const void *buf, usize len, int flags) {
    return sys_send(sockfd, buf, len, flags);
}

isize sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    return sys_sendmsg(sockfd, msg, flags);
}

isize sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
    return sys_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

isize recv(int sockfd, void *buf, usize len, int flags) {
    return sys_recv(sockfd, buf, len, flags);
}

isize recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return sys_recvmsg(sockfd, msg, flags);
}

isize recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *restrict src_addr, socklen_t *restrict addrlen) {
    return sys_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

int socket(int domain, int type, int protocol) {
    return sys_socket(domain, type, protocol);
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return sys_bind(sockfd, addr, addrlen);
}

int getsockname(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen) {
    return sys_getsockname(sockfd, addr, addrlen);
}

int listen(int sockfd, int backlog) {
    return sys_listen(sockfd, backlog);
}

int shutdown(int sockfd, int how) {
    return sys_shutdown(sockfd, how);
}

int getpeername(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen) {
    return sys_getpeername(sockfd, addr, addrlen);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *restrict optlen) {
    return sys_getsockopt(sockfd, level, optname, optval, optlen);
}

int sleep(const timespec_t *duration, timespec_t *rem) {
    return sys_nanosleep(duration, rem);
}

int gettimeofday(struct timeval *restrict tp, void *restrict tzp) {
    return sys_gettimeofday(tp, tzp);
}

int settimeofday(const struct timeval *tv, const struct timezone *tz) {
    return sys_settimeofday(tv, tz);
}

int clock_gettime(clockid_t clockid, struct timespec *tp) {
    return sys_clock_gettime(clockid, tp);
}

int clock_settime(clockid_t clockid, const struct timespec *tp) {
    return sys_clock_settime(clockid, tp);
}

int clock_getres(clockid_t clockid, struct timespec *res) {
    return sys_clock_getres(clockid, res);
}

int setrlimit(int resource, const void /*struct rlimit*/ *rlim) {
    return sys_setrlimit(resource, rlim);
}

int getrlimit(int resource, void /*struct rlimit*/ *rlim) {
    return sys_getrlimit(resource, rlim);
}

int getrusage(int who, void /*struct rusage*/ *usage) {
    return sys_getrusage(who, usage);
}
