#include <bits/errno.h>
#include <core/debug.h>
#include <sys/thread.h>

int sigaltstack(const uc_stack_t *ss, uc_stack_t *oss) {
    if (!current) {
        return -EINVAL;
    }

    // cannot modify the altsigstack if executing on altsigstack.
    if (current->t_arch.t_altstack.ss_flags & SS_ONSTACK) {
        return -EPERM;
    }
    
    // ss->ss_flags has flags other than SS_DISABLE set?
    if (ss && (ss->ss_flags & ~SS_DISABLE)) {
        return -EINVAL;
    }

    // size of altsigstack is too small.
    if (ss && (ss->ss_size < MINSIGSTKSZ)) {
        return -ENOMEM;
    }

    // return the current altsigstack.
    if (oss) {
        *oss = current->t_arch.t_altstack;
    }

    // set the new altsigstack.
    if (ss) {
        current->t_arch.t_altstack = *ss;
    }

    return 0;
}