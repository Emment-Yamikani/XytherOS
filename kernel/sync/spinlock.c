#include <sync/spinlock.h>


void spinlock_init(spinlock_t *lk) {
    spin_assert(lk);

    *lk = (spinlock_t) {
        .line   = 0,
        .file   = NULL,
        .owner  = NULL,
        .locked = false,
        .threaded = 0
    };
}

/* Call with lk->guard held and preemption disabled.*/
static int holding(spinlock_t *lk) {
    int self;
    spin_assert(lk);
    self = (lk)->threaded ? (lk)->owner == current : (lk)->owner == cpu;
    return (lk)->locked && self;
}

void spin_acquire(spinlock_t *lk) {
    spin_assert(lk);
    pushcli();
    memory_barrier();
    
    arch_raw_lock_acquire(&lk->guard);
    assert_eq(holding(lk), 0, "Spinlock already held @ [%s:%d].\n", lk->file, lk->line);

    loop() {
        if (lk->locked == 0)
            break;

        arch_raw_lock_release(&lk->guard);
        popcli();
        cpu_pause();
        pushcli();
        arch_raw_lock_acquire(&lk->guard);
    }

    lk->locked   = 1;
    lk->file     = __FILE__;
    lk->line     = __LINE__;
    lk->threaded = current ? 1 : 0;
    lk->owner    = current ? (void *)current : (void *)cpu;

    arch_raw_lock_release(&lk->guard);
    memory_barrier();
}

void spin_lock(spinlock_t *lk) {
    spin_assert(lk);
    spin_acquire(lk);
}

void spin_release(spinlock_t *lk) {
    spin_assert(lk);
    pushcli();
    memory_barrier();

    arch_raw_lock_acquire(&lk->guard);
    assert_eq(holding(lk), 1, "Spinlock must be held.\n");

    lk->locked   = 0;
    lk->line     = 0;
    lk->file     = NULL;
    lk->threaded = 0;
    lk->owner    = NULL;

    arch_raw_lock_release(&lk->guard);
    memory_barrier();
    popcli();
    popcli();
}

void spin_unlock(spinlock_t *lk) {
    spin_assert(lk);
    spin_release(lk);
}

int spin_islocked(spinlock_t *lk) {
    int locked = 0;

    spin_assert(lk);
    pushcli();
    arch_raw_lock_acquire(&lk->guard);
    memory_barrier();

    locked = holding(lk);

    arch_raw_lock_release(&lk->guard);
    memory_barrier();
    popcli();

    return locked;
}

int spin_test_and_lock(spinlock_t *lk) {
    spin_assert(lk);
    int locked;
    if ((locked = !spin_islocked(lk)))
        spin_lock(lk);
    return locked;
}

void spin_assert_locked(spinlock_t *lk) {
    spin_assert(lk);
    assert_eq(spin_islocked(lk), 1, "Spinlock must be held.\n");
}

int spin_trylock(spinlock_t *lk) {
    int not_locked = 1;

    spin_assert(lk);
    pushcli();
    memory_barrier();
    
    arch_raw_lock_acquire(&lk->guard);
    assert_eq(holding(lk), 0, "Spinlock already held @ [%s:%d].\n", lk->file, lk->line);

    if (lk->locked == 0) {
        not_locked   = 0;
        lk->locked   = 1;
        lk->file     = __FILE__;
        lk->line     = __LINE__;
        lk->threaded = current ? 1 : 0;
        lk->owner    = current ? (void *)current : (void *)cpu;
    } else {
        popcli();
    }

    arch_raw_lock_release(&lk->guard);
    memory_barrier();
    return not_locked;
}