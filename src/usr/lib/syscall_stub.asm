bits 64

section .text

%macro stub 2
global sys_%2
sys_%2:
    mov rax, %1
    int 0x80
    ret
%endmacro


%define SYS_kputc               0   ; void sys_kputc(int c);

%define SYS_close               1   ; int sys_close(int fd);
%define SYS_unlink              2   ; int sys_unlink(int fd);
%define SYS_dup                 3   ; int sys_dup(int fd);
%define SYS_dup2                4   ; int sys_dup2(int fd1, int fd2);
%define SYS_truncate            5   ; int sys_truncate(int fd, off_t length);
%define SYS_fcntl               6   ; int sys_fcntl(int fd, int cmd, void *argp);
%define SYS_ioctl               7   ; int sys_ioctl(int fd, int req, void *argp);
%define SYS_lseek               8   ; off_t sys_lseek(int fd, off_t off, int whence);
%define SYS_read                9   ; ssize_t sys_read(int fd, void *buf, size_t size);
%define SYS_write               10  ; ssize_t sys_write(int fd, void *buf, size_t size);
%define SYS_open                11  ; int sys_open(const char *pathname, int oflags, mode_t mode);
%define SYS_create              12  ; int sys_create(int fd, const char *filename, mode_t mode);
%define SYS_mkdirat             13  ; int sys_mkdirat(int fd, const char *filename, mode_t mode);
%define SYS_readdir             14  ; ssize_t sys_readdir(int fd, off_t off, void *buf, size_t count);
%define SYS_linkat              15  ; int sys_linkat(int fd, const char *oldname, const char *newname);
%define SYS_mknodat             16  ; int sys_mknodat(int fd, const char *filename, mode_t mode, int devid);
%define SYS_sync                17  ; int sys_sync(int fd);
%define SYS_mount               20  ; int sys_mount(const char *source, const char *target, const char *type, unsigned long flags, const void *data);
%define SYS_chmod               21  ; int sys_chmod(const char *pathname, mode_t mode);
%define SYS_rename              22  ; int sys_rename(const char *oldpath, const char *newpath);
%define SYS_chroot              23  ; int sys_chroot(const char *path);
%define SYS_utimes              24  ; int sys_utimes(const char *filename, const struct timeval times[2]);
%define SYS_pipe                25  ; int sys_pipe(int fds[2]);
%define SYS_fstat               26  ; int sys_fstat(int fildes, struct stat *buf);
%define SYS_stat                27  ; int sys_stat(const char *restrict path, struct stat *restrict buf);
%define SYS_lstat               28  ; int sys_lstat(const char *restrict path, struct stat *restrict buf);
%define SYS_fstatat             29  ; int sys_fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag);
%define SYS_chown               30  ; int sys_chown(const char *pathname, uid_t uid, gid_t gid);
%define SYS_fchown              31  ; int sys_fchown(int fd, uid_t uid, gid_t gid);
%define SYS_umask               32  ; mode_t sys_umask(mode_t cmask);
%define SYS_isatty              33  ; int sys_isatty(int fd);
%define SYS_mkdir               34  ; int sys_mkdir(const char *filename, mode_t mode);
%define SYS_mknod               35  ; int sys_mknod(const char *filename, mode_t mode, int devid);
%define SYS_getcwd              36  ; int sys_getcwd(char *buf, size_t size);
%define SYS_chdir               37  ; int sys_chdir(const char *path);
%define SYS_openat              38  ; int sys_openat(int fd, const char *pathname, int oflags, mode_t mode)
%define SYS_poll                39  ; int sys_poll(struct pollfd fds[] __unused, nfds_t nfds __unused, int timeout __unused);

%define SYS_getsid              50  ; pid_t sys_getsid(pid_t pid);
%define SYS_setsid              51  ; pid_t sys_setsid(void);
%define SYS_getpgrp             52  ; pid_t sys_getpgrp(void);
%define SYS_setpgrp             53  ; pid_t sys_setpgrp(void);
%define SYS_getpgid             54  ; pid_t sys_getpgid(pid_t pid);
%define SYS_setpgid             55  ; int sys_setpgid(pid_t pid, pid_t pgid);

%define SYS_fork                60  ; pid_t sys_fork(void);
%define SYS_getpid              61  ; pid_t sys_getpid(void);
%define SYS_getppid             62  ; pid_t sys_getppid(void);
%define SYS_exit                63  ; void sys_exit(int exit_code);
%define SYS_waitpid             64  ; pid_t sys_waitpid(pid_t __pid, int *__stat_loc, int __options);
%define SYS_ptrace              65  ; long sys_ptrace(enum __ptrace_request op, pid_t pid, void *addr, void *data);
%define SYS_execve              66  ; int sys_execve(const char *pathname, char *const argv[], char *const envp[]);
%define SYS_wait4               67  ; pid_t sys_wait4(pid_t pid, int *wstatus, int options, void /*struct rusage*/ *rusage);

%define SYS_park                80  ; int sys_park(void);
%define SYS_unpark              81  ; int sys_unpark(tid_t);
%define SYS_gettid              82  ; tid_t sys_gettid(void);
%define SYS_thread_exit         83  ; void sys_thread_exit(int exit_code);
%define SYS_thread_create       84  ; int sys_thread_create(tid_t *ptidp, void *attr, thread_entry_t entry, void *arg);
%define SYS_thread_join         85  ; int sys_thread_join(tid_t tid, void **retval);
%define SYS_set_thread_area     86  ; int sys_set_thread_area(void *addr);
%define SYS_get_thread_area     87  ; int sys_get_thread_area(void *addr);
%define SYS_thread_self         88  ; tid_t sys_thread_self(void)
%define SYS_thread_yield        89  ; void sys_thread_yield(void);

%define SYS_sigreturn           100 ; void sys_sigreturn();
%define SYS_pause               101 ; int  sys_pause(void);
%define SYS_alarm               102 ; uint sys_alarm(unsigned int sec);
%define SYS_sigpending          103 ; int  sys_sigpending(sigset_t *set);
%define SYS_kill                104 ; int  sys_kill(pid_t pid, int signo);
%define SYS_sigaction           105 ; int  sys_sigaction(int signo, const sigaction_t *act, sigaction_t *oact);
%define SYS_sigaltstack         106 ; int  sys_sigaltstack(const uc_stack_t *ss, uc_stack_t *oss);
%define SYS_sigprocmask         107 ; int  sys_sigprocmask(int how, const sigset_t *set, sigset_t *oset);
%define SYS_sigsuspend          108 ; int  sys_sigsuspend(const sigset_t *mask);
%define SYS_sigwaitinfo         109 ; int  sys_sigwaitinfo(const sigset_t *set, siginfo_t *info);
%define SYS_sigtimedwait        110 ; int  sys_sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout);

%define SYS_pthread_kill        111 ; int  sys_pthread_kill(tid_t thread, int signo);
%define SYS_pthread_sigmask     112 ; int  sys_pthread_sigmask(int how, const sigset_t *set, sigset_t *oset);
%define SYS_pthread_sigqueue    113 ; int  sys_pthread_sigqueue(tid_t tid, int signo, union sigval sigval);


%define SYS_getpagesize         130  ; int sys_getpagesize(void);
%define SYS_getmemusage         131  ; void sys_getmemusage(meminfo_t *info);
%define SYS_mmap                132  ; void *sys_mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
%define SYS_munmap              133  ; int sys_munmap(void *addr, size_t len);
%define SYS_mprotect            134  ; int sys_mprotect(void *addr, size_t len, int prot);
%define SYS_mlock               135  ; int sys_mlock(const void *addr, size_t len);
%define SYS_mlock2              136  ; int sys_mlock2(const void *addr, size_t len, unsigned int flags);
%define SYS_munlock             137  ; int sys_munlock(const void *addr, size_t len);
%define SYS_mlockall            138  ; int sys_mlockall(int flags);
%define SYS_munlockall          139  ; int sys_munlockall(void);
%define SYS_msync               140  ; int sys_msync(void *addr, size_t length, int flags);
%define SYS_mremap              141  ; void *sys_mremap(void *old_address, size_t old_size, size_t new_size, int flags, ... /* void *new_address */);
%define SYS_madvise             142  ; int sys_madvise(void *addr, size_t length, int advice);
%define SYS_sbrk                143  ; void *sys_sbrk(intptr_t increment);

%define SYS_uname               150  ; int sys_uname(struct utsname *name);

%define SYS_getuid              151  ; uid_t sys_getuid(void);
%define SYS_getgid              152  ; gid_t sys_getgid(void);
%define SYS_geteuid             153  ; uid_t sys_geteuid(void);
%define SYS_getegid             154  ; gid_t sys_getegid(void);
%define SYS_setuid              155  ; int sys_setuid(uid_t uid);
%define SYS_setgid              156  ; int sys_setgid(gid_t gid);
%define SYS_seteuid             157  ; int sys_seteuid(uid_t euid);
%define SYS_setegid             158  ; int sys_setegid(gid_t egid);

%define SYS_connect             160  ; int sys_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
%define SYS_accept              161  ; int sys_accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
%define SYS_send                162  ; isize sys_send(int sockfd, const void *buf, usize len, int flags);
%define	SYS_sendmsg             163  ; isize sys_sendmsg(int sockfd, const struct msghdr *msg, int flags);
%define SYS_sendto              164  ; isize sys_sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
%define SYS_recv                165  ; isize sys_recv(int sockfd, void *buf, usize len, int flags);
%define SYS_recvmsg             166  ; isize sys_recvmsg(int sockfd, struct msghdr *msg, int flags);
%define SYS_recvfrom            167  ; isize sys_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *restrict src_addr, socklen_t *restrict addrlen);
%define SYS_socket              168  ; int sys_socket(int domain, int type, int protocol);
%define SYS_bind                169  ; int sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
%define SYS_getsockname         170  ; int sys_getsockname(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
%define SYS_listen              171  ; int sys_listen(int sockfd, int backlog);
%define SYS_shutdown            172  ; int sys_shutdown(int sockfd, int how);
%define SYS_getpeername         173  ; int sys_getpeername(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
%define SYS_getsockopt          174  ; int sys_getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *restrict optlen);

%define SYS_nanosleep           181  ; int sys_nanosleep(const timespec_t *duration, timespec_t *rem);
%define SYS_gettimeofday        182  ; int sys_gettimeofday(struct timeval *restrict tp, void *restrict tzp);
%define SYS_settimeofday        183  ; int sys_settimeofday(const struct timeval *tv, const struct timezone *tz);
%define SYS_clock_gettime       184  ; int sys_clock_gettime(clockid_t clockid, struct timespec *tp);
%define SYS_clock_settime       185  ; int sys_clock_settime(clockid_t clockid, const struct timespec *tp);
%define SYS_clock_getres        186  ; int sys_clock_getres(clockid_t clockid, struct timespec *res);

%define SYS_setrlimit           200  ; int sys_setrlimit(int resource, const void /*struct rlimit*/ *rlim);
%define SYS_getrlimit           201  ; int sys_getrlimit(int resource, void /*struct rlimit*/ *rlim);
%define SYS_getrusage           202  ; int sys_getrusage(int who, void /*struct rusage*/ *usage);


stub SYS_kputc,             kputc

stub SYS_close,             close
stub SYS_unlink,            unlink
stub SYS_dup,               dup
stub SYS_dup2,              dup2
stub SYS_truncate,          truncate
stub SYS_fcntl,             fcntl
stub SYS_ioctl,             ioctl
stub SYS_lseek,             lseek
stub SYS_read,              read
stub SYS_write,             write
stub SYS_open,              open
stub SYS_create,            create
stub SYS_mkdirat,           mkdirat
stub SYS_readdir,           readdir
stub SYS_linkat,            linkat
stub SYS_mknodat,           mknodat
stub SYS_sync,              sync
stub SYS_mount,             mount
stub SYS_chmod,             chmod
stub SYS_rename,            rename
stub SYS_chroot,            chroot
stub SYS_utimes,            utimes
stub SYS_pipe,              pipe
stub SYS_fstat,             fstat
stub SYS_stat,              stat
stub SYS_lstat,             lstat
stub SYS_fstatat,           fstatat
stub SYS_chown,             chown
stub SYS_fchown,            fchown
stub SYS_umask,             umask
stub SYS_isatty,            isatty
stub SYS_mkdir,             mkdir
stub SYS_mknod,             mknod
stub SYS_getcwd,            getcwd
stub SYS_chdir,             chdir
stub SYS_openat,            openat
stub SYS_poll,              poll

stub SYS_getsid,            getsid
stub SYS_setsid,            setsid
stub SYS_getpgrp,           getpgrp
stub SYS_setpgrp,           setpgrp
stub SYS_getpgid,           getpgid
stub SYS_setpgid,           setpgid

stub SYS_fork,              fork
stub SYS_getpid,            getpid
stub SYS_getppid,           getppid
stub SYS_exit,              exit
stub SYS_waitpid,           waitpid
stub SYS_ptrace,            ptrace
stub SYS_execve,            execve
stub SYS_wait4,             wait4

stub SYS_park,              park
stub SYS_unpark,            unpark
stub SYS_gettid,            gettid
stub SYS_thread_exit,       thread_exit
stub SYS_thread_create,     thread_create
stub SYS_thread_join,       thread_join
stub SYS_set_thread_area,   set_thread_area
stub SYS_get_thread_area,   get_thread_area
stub SYS_thread_self,       thread_self
stub SYS_thread_yield,      thread_yield

stub SYS_sigreturn,         sigreturn
stub SYS_pause,             pause
stub SYS_alarm,             alarm
stub SYS_sigpending,        sigpending
stub SYS_kill,              kill
stub SYS_sigaction,         sigaction
stub SYS_sigaltstack,       sigaltstack
stub SYS_sigprocmask,       sigprocmask
stub SYS_sigsuspend,        sigsuspend
stub SYS_sigwaitinfo,       sigwaitinfo
stub SYS_sigtimedwait,      sigtimedwait

stub SYS_pthread_kill,      pthread_kill
stub SYS_pthread_sigmask,   pthread_sigmask
stub SYS_pthread_sigqueue,  pthread_sigqueue


stub SYS_getpagesize,       getpagesize
stub SYS_getmemusage,       getmemusage
stub SYS_mmap,              mmap
stub SYS_munmap,            munmap
stub SYS_mprotect,          mprotect
stub SYS_mlock,             mlock
stub SYS_mlock2,            mlock2
stub SYS_munlock,           munlock
stub SYS_mlockall,          mlockall
stub SYS_munlockall,        munlockall
stub SYS_msync,             msync
stub SYS_mremap,            mremap
stub SYS_madvise,           madvise
stub SYS_sbrk,              sbrk

stub SYS_uname,             uname

stub SYS_getuid,            getuid
stub SYS_getgid,            getgid
stub SYS_geteuid,           geteuid
stub SYS_getegid,           getegid
stub SYS_setuid,            setuid
stub SYS_setgid,            setgid
stub SYS_seteuid,           seteuid
stub SYS_setegid,           setegid

stub SYS_connect,           connect
stub SYS_accept,            accept
stub SYS_send,              send
stub SYS_sendmsg,           sendmsg
stub SYS_sendto,            sendto
stub SYS_recv,              recv
stub SYS_recvmsg,           recvmsg
stub SYS_recvfrom,          recvfrom
stub SYS_socket,            socket
stub SYS_bind,              bind
stub SYS_getsockname,       getsockname
stub SYS_listen,            listen
stub SYS_shutdown,          shutdown
stub SYS_getpeername,       getpeername
stub SYS_getsockopt,        getsockopt

stub SYS_nanosleep,         nanosleep
stub SYS_gettimeofday,      gettimeofday
stub SYS_settimeofday,      settimeofday
stub SYS_clock_gettime,     clock_gettime
stub SYS_clock_settime,     clock_settime
stub SYS_clock_getres,      clock_getres

stub SYS_setrlimit,         setrlimit
stub SYS_getrlimit,         getrlimit
stub SYS_getrusage,         getrusage