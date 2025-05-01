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

bool holding(const spinlock_t *lk) {
    spin_assert(lk);
    bool intena = disable_interrupts();
    bool locked = lk->locked ? true : false;
    locked = locked && (lk->owner == current || (lk)->owner == cpu);
    enable_interrupts(intena);
    return locked;
}

#if defined(USE_SPINLOCK_FUNCTIONS)
void spin_acquire(spinlock_t *lk) {
    spin_assert(lk);
    pushcli();

    arch_raw_lock_acquire(&(lk)->guard);
    assert_eq(holding(lk), false,
              "Spinlock already held by [%p] @ [%s:%d].\n",
              (lk)->owner, (lk)->file, (lk)->line);

    loop() {
        if ((lk)->locked == 0) {
            break;
        }

        arch_raw_lock_release(&(lk)->guard);
        popcli();
        cpu_pause();
        pushcli();
        arch_raw_lock_acquire(&(lk)->guard);
    }

    (lk)->locked = 1;
    (lk)->file = __FILE__;
    (lk)->line = __LINE__;
    (lk)->owner = current ? (void *)current : (void *)cpu;

    arch_raw_lock_release(&(lk)->guard);
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

    arch_raw_lock_acquire(&(lk)->guard);
    assert_eq(holding(lk), true, "Spinlock must be held.\n");

    (lk)->line = 0;
    (lk)->file = NULL;
    (lk)->owner = NULL;
    (lk)->locked = 0;
    arch_raw_lock_release(&(lk)->guard);
    popcli();
    popcli();
}

void spin_unlock(spinlock_t *lk) {
    spin_assert(lk);
    spin_release(lk);
}

bool spin_islocked(spinlock_t *lk) {
    bool locked = 0;

    spin_assert(lk);
    pushcli();
    arch_raw_lock_acquire(&(lk)->guard);
    locked = holding(lk);
    arch_raw_lock_release(&(lk)->guard);
    popcli();
    return locked;
}

bool spin_recursive_lock(spinlock_t *lk) {
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

bool spin_trylock(spinlock_t *lk) {
    bool success = 0;

    spin_assert(lk);
    pushcli();

    arch_raw_lock_acquire(&(lk)->guard);
    assert_eq(holding(lk), 0, "Spinlock already held @ [%s:%d].\n", (lk)->file, (lk)->line);

    if ((lk)->locked == 0) {
        success = 1;
        (lk)->locked = 1;
        (lk)->file = __FILE__;
        (lk)->line = __LINE__;
        (lk)->owner = current ? (void *)current : (void *)cpu;
    }
    else
    {
        popcli();
    }

    arch_raw_lock_release(&(lk)->guard);
    memory_barrier();
    return success;
}
#endif // USE_SPINLOCK_FUNCTIONS