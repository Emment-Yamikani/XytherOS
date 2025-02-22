#include <sync/spinlock.h>


void spinlock_init(spinlock_t *lk) {
    spin_assert(lk);

    *lk = (spinlock_t) {
        .line   = 0,
        .file   = NULL,
        .owner  = NULL,
        .locked = false,
    };

    arch_raw_lock_init(&lk->guard);
}
