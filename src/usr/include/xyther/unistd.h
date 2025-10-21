#include <xyther/types.h>
#include <xyther/bits/stat.h>
#include <xyther/bits/fcntl.h>
#include <xyther/signal.h>
#include <xyther/time.h>
#include <xyther/socket.h>
#include <xyther/poll.h>
#include <xyther/ptrace.h>
#include <xyther/utsname.h>

extern void kputc(int c);
extern int close(int fd);
extern int unlink(int fd);
extern int dup(int fd);
extern int dup2(int fd1, int fd2);
extern int truncate(int fd, off_t length);
extern int fcntl(int fd, int cmd, void *argp);
extern int ioctl(int fd, int req, void *argp);
extern off_t lseek(int fd, off_t off, int whence);
extern ssize_t read(int fd, void *buf, size_t size);
extern ssize_t write(int fd, const void *buf, size_t size);
extern int open(const char *pathname, int oflags, mode_t mode);
extern int create(const char *filename, mode_t mode);
extern int mkdirat(int fd, const char *filename, mode_t mode);
extern ssize_t readdir(int fd, off_t off, void *buf, size_t count);
extern int linkat(int fd, const char *oldname, const char *newname);
extern int mknodat(int fd, const char *filename, mode_t mode, int devid);
extern int sync(int fd);
extern int mount(const char *source, const char *target, const char *type, unsigned long flags, const void *data);
extern int chmod(const char *pathname, mode_t mode);
extern int rename(const char *oldpath, const char *newpath);
extern int chroot(const char *path);
extern int utimes(const char *filename, const struct timeval times[2]);
extern int pipe(int fds[2]);
extern int fstat(int fildes, struct stat *buf);
extern int stat(const char *restrict path, struct stat *restrict buf);
extern int lstat(const char *restrict path, struct stat *restrict buf);
extern int fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag);
extern int chown(const char *pathname, uid_t uid, gid_t gid);
extern int fchown(int fd, uid_t uid, gid_t gid);
extern mode_t umask(mode_t cmask);
extern int isatty(int fd);
extern int mkdir(const char *filename, mode_t mode);
extern int mknod(const char *filename, mode_t mode, int devid);
extern int getcwd(char *buf, size_t size);
extern int chdir(const char *path);
extern int openat(int fd, const char *pathname, int oflags, mode_t mode);
extern int poll(struct pollfd fds[], nfds_t nfds, int timeout);

extern pid_t getsid(pid_t pid);
extern pid_t setsid(void);
extern pid_t getpgrp(void);
extern pid_t setpgrp(void);
extern pid_t getpgid(pid_t pid);
extern int setpgid(pid_t pid, pid_t pgid);
extern pid_t fork(void);
extern pid_t getpid(void);
extern pid_t getppid(void);
extern void exit(int status);
extern pid_t waitpid(pid_t __pid, int *__stat_loc, int __options);
extern long ptrace(enum __ptrace_request op, pid_t pid, void *addr, void *data);
extern int execve(const char *pathname, char *const argv[], char *const envp[]);
extern pid_t wait4(pid_t pid, int *wstatus, int options, void /*struct rusage*/ *rusage);

extern int park(void);
extern int unpark(tid_t);
extern tid_t gettid(void);
extern void thread_exit(int status);
extern int thread_create(tid_t *ptidp, void *attr, thread_entry_t entry, void *arg);
extern int thread_join(tid_t tid, void **retval);
extern int set_thread_area(void *addr);
extern int get_thread_area(void *addr);
extern tid_t thread_self(void);
extern void thread_yield(void);

extern void sigreturn();
extern int pause(void);
extern unsigned int alarm(unsigned int sec);
extern int sigpending(sigset_t *set);
extern int kill(pid_t pid, int signo);
extern int sigaction(int signo, const sigaction_t *act, sigaction_t *oact);
extern int sigaltstack(const uc_stack_t *ss, uc_stack_t *oss);
extern int sigprocmask(int how, const sigset_t *set, sigset_t *oset);
extern int sigsuspend(const sigset_t *mask);
extern int sigwaitinfo(const sigset_t *set, siginfo_t *info);
extern int sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout);
extern int pthread_kill(tid_t thread, int signo);
extern int pthread_sigmask(int how, const sigset_t *set, sigset_t *oset);
extern int pthread_sigqueue(tid_t tid, int signo, union sigval sigval);

extern int getpagesize(void);
extern int getmemstats(mem_stats_t *info);
extern void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
extern int munmap(void *addr, size_t len);
extern int mprotect(void *addr, size_t len, int prot);
extern int mlock(const void *addr, size_t len);
extern int mlock2(const void *addr, size_t len, unsigned int flags);
extern int munlock(const void *addr, size_t len);
extern int mlockall(int flags);
extern int munlockall(void);
extern int msync(void *addr, size_t length, int flags);
extern void *mremap(void *old_address, size_t old_size, size_t new_size, int flags, ... /* void *new_address */);
extern int madvise(void *addr, size_t length, int advice);
extern void *sbrk(intptr_t increment);

extern int uname(struct utsname *name);

extern uid_t getuid(void);
extern gid_t getgid(void);
extern uid_t geteuid(void);
extern gid_t getegid(void);
extern int setuid(uid_t uid);
extern int setgid(gid_t gid);
extern int seteuid(uid_t euid);
extern int setegid(gid_t egid);

extern int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern int accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
extern isize send(int sockfd, const void *buf, usize len, int flags);
extern isize sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
extern isize recv(int sockfd, void *buf, usize len, int flags);
extern isize sendmsg(int sockfd, const struct msghdr *msg, int flags);
extern isize recvmsg(int sockfd, struct msghdr *msg, int flags);
extern isize recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *restrict src_addr, socklen_t *restrict addrlen);
extern int socket(int domain, int type, int protocol);
extern int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern int getsockname(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
extern int listen(int sockfd, int backlog);
extern int shutdown(int sockfd, int how);
extern int getpeername(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
extern int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *restrict optlen);

extern int nanosleep(const timespec_t *duration, timespec_t *rem);
extern int gettimeofday(struct timeval *restrict tp, void *restrict tzp);
extern int settimeofday(const struct timeval *tv, const struct timezone *tz);
extern int clock_gettime(clockid_t clockid, struct timespec *tp);
extern int clock_settime(clockid_t clockid, const struct timespec *tp);
extern int clock_getres(clockid_t clockid, struct timespec *res);

extern int setrlimit(int resource, const void /*struct rlimit*/ *rlim);
extern int getrlimit(int resource, void /*struct rlimit*/ *rlim);
extern int getrusage(int who, void /*struct rusage*/ *usage);