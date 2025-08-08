#pragma once

#include <bits/errno.h>
#include <core/types.h>

typedef enum signo_t {
    SIGABRT     = 1,  // abnormal termination (abort)         | terminate+core.
    SIGALRM     = 2,  // timer expired (alarm)                | terminate.
    SIGBUS      = 3,  // hardware fault                       | terminate+core.
    SIGCANCEL   = 4,  // threads library internal use         | ignore.
    SIGCHLD     = 5,  // change in status of child            | ignore.
    SIGCONT     = 6,  // continue stopped process             | continue/ignore.
    SIGEMT      = 7,  // hardware fault                       | terminate+core.
    SIGFPE      = 8,  // arithmetic exception                 | terminate+core.
    SIGHUP      = 9,  // hangup                               | terminate.
    SIGILL      = 10, // illegal instruction                  | terminate+core.
    SIGINT      = 11, // terminal interrupt character         | terminate.
    SIGIO       = 12, // asynchronous I/O                     | terminate/ignore.
    SIGIOT      = 13, // hardware fault                       | terminate+core.
    SIGKILL     = 14, // termination                          | terminate.
    SIGPIPE     = 15, // write to pipe with no readers        | terminate.
    SIGPROF     = 16, // profiling time alarm (setitimer)     | terminate.
    SIGQUIT     = 17, // terminal quit character              | terminate+core.
    SIGSEGV     = 18, // invalid memory reference             | terminate+core.
    SIGSTOP     = 19, // stop                                 | stop process.
    SIGSYS      = 20, // invalid system call                  | terminate+core.
    SIGTERM     = 21, // termination                          | terminate.
    SIGTRAP     = 22, // hardware fault                       | terminate+core.
    SIGTSTP     = 23, // terminal stop character              | stop process.
    SIGTTIN     = 24, // background read from control tty     | stop process.
    SIGTTOU     = 25, // background write to control tty      | stop process.
    SIGURG      = 26, // urgent condition (sockets)           | ignore.
    SIGUSR1     = 27, // user-defined signal                  | terminate.
    SIGUSR2     = 28, // user-defined signal                  | terminate.
    SIGVTALRM   = 29, // virtual time alarm (setitimer)       | terminate.
    SIGWINCH    = 30, // terminal window size change          | ignore.
    SIGXCPU     = 31, // CPU limit exceeded (setrlimit)       | terminate or terminate+core.
    SIGXFSZ     = 32, // file size limit exceeded (setrlimit) | terminate or terminate+core.
} signo_t;

#ifndef NSIG
#define NSIG    32
#endif

#define SIGMASK(signo)      (1ul << (signo - 1))
#define SIG_INDEX(signo)    (((signo) - 1) / __BITS_PER_UINT)
#define SIGBAD(signo)       ((((signo) < 1) || ((signo) > NSIG)))
#define SIG_BIT(signo)      (1u << (((signo) - 1) % __BITS_PER_UINT))

//bits per unsigned int.
#define __BITS_PER_UINT (sizeof(uint) * 8)
#define __NR_INT        (__BITS_PER_UINT / NSIG)

typedef struct {
    uint sigset[__NR_INT];
} sigset_t, __sigset_t;


static inline int sigsetadd(sigset_t *set, int signo) {
    if (SIGBAD(signo)) {
        return -EINVAL;
    }
    set->sigset[SIG_INDEX(signo)] |= SIG_BIT(signo);
    return 0;
}

static inline int sigsetdel(sigset_t *set, int signo) {
    if (SIGBAD(signo)) {
        return -EINVAL;
    }
    set->sigset[SIG_INDEX(signo)] &= ~SIG_BIT(signo);
    return 0;
}

static inline int sigismember(const sigset_t *set, int signo) {
    if (SIGBAD(signo)) {
        return -EINVAL;
    }
    return (set->sigset[SIG_INDEX(signo)] & SIG_BIT(signo)) ? 1 : 0;
}

static inline int sigset_first(sigset_t *set) {
    for (usize i = 0; i < __NR_INT; ++i) {
        if (set->sigset[i]) {
            return i * __BITS_PER_UINT + __builtin_ctz(set->sigset[i]) + 1;
        }
    }
    return 0; // No signal found
}

static inline void sigsetempty(sigset_t *set) {
    set->sigset[0] = 0;
    // If __NR_INT > 1, then loop over remaining indices
    for (usize i = 1; i < __NR_INT; ++i) {
        set->sigset[i] = 0;
    }
}

static inline void sigsetfill(sigset_t *set) {
    set->sigset[0] = ~0U;
    // If __NR_INT > 1, then loop over remaining indices
    for (usize i = 1; i < __NR_INT; ++i) {
        set->sigset[i] = 0;
    }
}

static inline void sigsetaddmask(sigset_t *set, uint mask) {
    set->sigset[0] |= mask;
}

static inline void sigsetdelmask(sigset_t *set, uint mask) {
    set->sigset[0] &= ~mask;
}

static inline void sigsetor(const sigset_t *set, const sigset_t *set1, sigset_t *out) {
    for (usize i = 0; i < __NR_INT; ++i) {
        out->sigset[i] = set->sigset[i] | set1->sigset[i];
    }
}

static inline void sigsetand(const sigset_t *set, const sigset_t *set1, sigset_t *out) {
    for (usize i = 0; i < __NR_INT; ++i) {
        out->sigset[i] = set->sigset[i] & set1->sigset[i];
    }
}

static inline void sigsettest(const sigset_t *set, const sigset_t *set1, sigset_t *out) {
    for (usize i = 0; i < __NR_INT; ++i) {
        out->sigset[i] = set->sigset[i] & set1->sigset[i];
    }
}