#include <sync/rwlock.h>
#include <sys/schedule.h>
#include <sys/thread.h>

/* 
 * Initialize the rwlock.
 * Must be called before the lock is used.
 */
void rwlock_init(rwlock_t *rw) {
    rwlock_assert(rw);

    atomic_set(&rw->writer, 0);
    atomic_set(&rw->readers, 0);
    queue_init(&rw->writersq);
    queue_init(&rw->readersq);
    spinlock_init(&rw->guard);
}

/*
 * Acquire the read lock.
 * Blocks if a writer is active or waiting.
 */
void rw_readlock_acquire(rwlock_t *rw) {
    rwlock_assert(rw);

    spin_lock(&rw->guard);

    // Wait if a writer is active or if there are writers waiting.
    while (atomic_read(&rw->writer) || sched_wait_queue_length(&rw->writersq)) {
        // Sleep and add ourselves to the readers queue.
        sched_wait_whence(&rw->readersq, T_SLEEP, QUEUE_TAIL, &rw->guard);
    }

    // Once safe, increment the readers counter.
    atomic_inc(&rw->readers);
    spin_unlock(&rw->guard);
}

/*
 * Try to acquire the read lock.
 *
 * Returns nonzero (true) if the lock was acquired, or 0 if the lock cannot be
 * acquired immediately (because a writer is active or waiting).
 */
int rw_readlock_try_acquire(rwlock_t *rw) {
    int success = 0;
    rwlock_assert(rw);
    spin_lock(&rw->guard);

    // Only succeed if no writer is active and no writer is waiting.
    if (atomic_read(&rw->writer) == 0 && sched_wait_queue_length(&rw->writersq) == 0) {
        atomic_inc(&rw->readers);
        success = 1;
    }
    spin_unlock(&rw->guard);
    return success;
}


/*
 * Release the read lock.
 * Wakes a waiting writer if this is the last reader.
 */
void rw_readlock_release(rwlock_t *rw) {
    rwlock_assert(rw);

    spin_lock(&rw->guard);

    // Decrement readers counter; if no more readers, consider waking a writer.
    if (atomic_dec_fetch(&rw->readers) == 0) {
        if (sched_wait_queue_length(&rw->writersq) >= 1) {
            // Wake up one waiting writer.
            sched_wakeup_whence(&rw->writersq, WAKEUP_NORMAL, QUEUE_HEAD);
        }
    }
    spin_unlock(&rw->guard);
}

/*
 * Acquire the write lock.
 * Blocks if there are active readers, an active writer, or waiting writers.
 * This makes it fair for writers (FCFS).
 */
void rw_writelock_acquire(rwlock_t *rw) {
    rwlock_assert(rw);

    spin_lock(&rw->guard);

    while (atomic_read(&rw->readers) || atomic_read(&rw->writer) ||
           sched_wait_queue_length(&rw->writersq)) {
        // Wait and add ourselves to the writer queue.
        sched_wait_whence(&rw->writersq, T_SLEEP, QUEUE_TAIL, &rw->guard);
    }

    // Mark writer active.
    atomic_inc(&rw->writer);
    spin_unlock(&rw->guard);
}

/*
 * Try to acquire the write lock.
 *
 * Returns nonzero (true) if the write lock was acquired immediately, or 0
 * if the lock is contended.
 */
int rw_writelock_try_acquire(rwlock_t *rw) {
    int success = 0;
    rwlock_assert(rw);
    spin_lock(&rw->guard);

    // Succeed only if no active readers, no active writer,
    // and no other writer is waiting.
    if (atomic_read(&rw->readers) == 0 &&
        atomic_read(&rw->writer) == 0 &&
        sched_wait_queue_length(&rw->writersq) == 0) {
        atomic_inc(&rw->writer);
        success = 1;
    }
    spin_unlock(&rw->guard);
    return success;
}

/*
 * Release the write lock.
 * Preferentially wakes a waiting writer; if none, wakes all waiting readers.
 */
void rw_writelock_release(rwlock_t *rw) {
    rwlock_assert(rw);

    spin_lock(&rw->guard);

    // Clear the writer indicator.
    atomic_dec(&rw->writer);

    // First, if any writer is waiting, wake one up.
    if (sched_wait_queue_length(&rw->writersq)) {
        sched_wakeup_whence(&rw->writersq, WAKEUP_NORMAL, QUEUE_HEAD);
        spin_unlock(&rw->guard);
        return;
    }

    // Otherwise, wake all waiting readers.
    sched_wakeup_all(&rw->readersq, WAKEUP_NORMAL, NULL);
    spin_unlock(&rw->guard);
}