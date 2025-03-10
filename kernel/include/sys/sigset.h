#pragma once

#include <bits/errno.h>
#include <core/types.h>

typedef int signo_t;

#ifndef NSIG
#define NSIG    32
#endif

#define SIGMASK(signo)      (1ul << (signo - 1))
#define SIG_INDEX(signo)    (((signo) - 1) / __BITS_PER_UINT)
#define SIGBAD(signo)       ({ ((signo) < 1 || (signo) > NSIG); })
#define SIG_BIT(signo)      (1u << (((signo) - 1) % __BITS_PER_UINT))

//bits per unsigned int.
#define __BITS_PER_UINT (sizeof(uint) * 8)
#define __NR_INT        (__BITS_PER_UINT / NSIG)

typedef struct {
    uint sigset[__NR_INT];
} sigset_t, __sigset_t;


static inline int sigsetadd(sigset_t *set, int signo) {
    if (SIGBAD(signo))
        return -EINVAL;
    set->sigset[SIG_INDEX(signo)] |= SIG_BIT(signo);
    return 0;
}

static inline int sigsetdel(sigset_t *set, int signo) {
    if (SIGBAD(signo))
        return -EINVAL;
    set->sigset[SIG_INDEX(signo)] &= ~SIG_BIT(signo);
    return 0;
}

static inline int sigismember(const sigset_t *set, int signo) {
    if (SIGBAD(signo))
        return -EINVAL;
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
    for (usize i = 1; i < __NR_INT; ++i)
        set->sigset[1] = 0;
}

static inline void sigsetfill(sigset_t *set) {
    set->sigset[0] = ~0U;
    // If __NR_INT > 1, then loop over remaining indices
    for (usize i = 1; i < __NR_INT; ++i)
        set->sigset[1] = 0;
}

static inline void sigsetaddmask(sigset_t *set, uint mask) {
    set->sigset[0] |= mask;
}

static inline void sigsetdelmask(sigset_t *set, uint mask) {
    set->sigset[0] &= ~mask;
}