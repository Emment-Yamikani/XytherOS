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
    
    if (ss != NULL) {
        // ss->ss_flags has flags other than SS_DISABLE set?
        if ((ss->ss_flags & ~SS_DISABLE) || ((ss->ss_size < MINSIGSTKSZ))) {
            return -EINVAL;
        }
        
        // return the current altsigstack.
        if (oss) {
            *oss = current->t_arch.t_altstack;
        }

        // set the new altsigstack.
        current->t_arch.t_altstack = *ss;
    }

    return 0;
}