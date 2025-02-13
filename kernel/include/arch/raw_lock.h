#pragma once

/**
 * @file arch_raw_lock.h
 * @brief Architecture-specific spinlock interface.
 *
 * This header declares a simple spinlock interface implemented with atomic
 * operations in assembly. These locks are intended for low-level synchronization
 * in performance-critical sections.
 */

/// The type representing a raw spinlock.
///
/// Typically a 32-bit integer where 0 indicates an unlocked state and
/// nonzero indicates a locked state.
typedef int arch_raw_lock_t;

/**
 * @brief Acquires the spinlock.
 *
 * This function implements a busy-wait loop (spinlock) that repeatedly tries
 * to acquire the lock using an atomic exchange. It will loop until the lock
 * is successfully acquired.
 *
 * @param lk Pointer to the lock variable.
 */
extern void arch_raw_lock_acquire(arch_raw_lock_t *lk);

/**
 * @brief Releases the spinlock.
 *
 * This function releases the lock by setting the value back to 0 using an
 * atomic operation. It is assumed that the calling thread currently holds the lock.
 *
 * @param lk Pointer to the lock variable.
 */
extern void arch_raw_lock_release(arch_raw_lock_t *lk);

/**
 * @brief Attempts to acquire the spinlock without blocking.
 *
 * This function performs a single atomic exchange on the lock variable.
 * If the lock was free (value 0), it is acquired and the function returns 0.
 * If the lock was already held (nonzero), the function returns a nonzero value.
 *
 * @param lk Pointer to the lock variable.
 * @return int 0 if the lock was acquired; nonzero otherwise.
 */
extern int arch_raw_lock_trylock(arch_raw_lock_t *lk);
