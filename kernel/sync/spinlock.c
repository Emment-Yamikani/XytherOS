#include <sync/spinlock.h>

void spinlock_dump(spinlock_t *lk) {
    printk(
        "guard: %d\n"
        "locked:%d\n"
        "lockat:%s:%d\n"
        "owner :%p\n",
        lk->guard,
        lk->locked,
        lk->file,
        lk->line,
        lk->owner
    );
}

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

int holding(const spinlock_t *lk) {
    spin_assert(lk);
    return lk->locked && (lk->owner == current || (lk)->owner == cpu);
}