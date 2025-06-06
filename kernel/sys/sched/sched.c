#include <bits/errno.h>
#include <core/debug.h>
#include <sys/schedule.h>
#include <sys/thread.h>

// global sleep queue.
static QUEUE(global_sleep_queue);


void sched_promote_thread(thread_sched_t *tsched) {
    int pr = tsched->ts_prio; // current priority level.

    if ((pr < MLFQ_HIGH) && (tsched->ts_short_holds >= TH_SHORTHOLD_THRESHOLD(pr))) {
        tsched->ts_prio += 1;
        tsched->ts_short_holds = 0; // decay over time.
        tsched->ts_long_holds  = 0; // decay over time.
    } else {
        tsched->ts_short_holds++;
    }
}

void sched_demote_thread(thread_sched_t *tsched) {
    int pr = tsched->ts_prio; // current priority level.

    if ((pr > MLFQ_LOW) && (tsched->ts_long_holds >= TH_LONGHOLD_THRESHOLD)) {
        tsched->ts_prio -= 1;
        tsched->ts_short_holds = 0; // decay over time.
        tsched->ts_long_holds  = 0; // decay over time.
    } else {
        tsched->ts_long_holds++;
    }
}

void sched_set_highest_prior(thread_sched_t *tsched) {
    tsched->ts_long_holds   = 0;
    tsched->ts_short_holds  = 0;
    tsched->ts_prio         = MLFQ_HIGH;
}

void sched(void) {
    isize ncli  = 1; // Don't change this, must always be == 1.
    bool intena = 0;
    
    current_assert_locked();
    
    if (current_test(THREAD_WAKE)) {
        current_mask(THREAD_WAKE | THREAD_PARK);
        return;
    }
    
    pushcli();
    cpu_swap_preepmpt(&ncli, &intena);
    
    thread_sched_t *ts = &current->t_info.ti_sched;
    if (ts->ts_flags & TS_SCHEDULER) { // scheduler thread always runs at highest priority.
        sched_set_highest_prior(ts);
    } else if (current_gettimeslice() == 0) { // If not used up entire timeslice, demote.
        sched_demote_thread(ts);
    } else sched_promote_thread(ts); // promote thread for cooperative preemption.

    // Return to the scheduler.
    context_switch(&current->t_arch.t_context);

    cpu_swap_preepmpt(&ncli, &intena);
    popcli();
}

/**
 * @brief 
 * 
 */
void sched_yield(void) {
    current_lock();
    current_enter_state(T_READY);
    sched();
    current_unlock();
}

/**
 * @brief 
 * 
 * @param wait_queue 
 * @param state 
 * @param whence 
 * @return int 
 */
static int sched_wait_validate_input(queue_t *wait_queue, tstate_t state) {
    if (wait_queue == NULL || current == NULL) {
        return -EINVAL;
    }

    // Thread can only enter a wait state as SLEEPING or STOPPED.
    if (state != T_SLEEP && state != T_STOPPED) {
        return -EINVAL;
    }

    return 0;
}

static inline void sched_block(spinlock_t *lock) {
    if (lock) {
        spin_unlock(lock); // releasse the lock.
    }

    sched();

    if (lock) {
        spin_lock(lock);   // re-acquire the lock.
    }
} 

/**
 * @brief 
 * 
 * @param wait_queue 
 * @param state 
 * @param whence 
 * @param lock 
 * @return int 
 */
int sched_wait_whence(queue_t *wait_queue, tstate_t state, queue_relloc_t whence, spinlock_t *lock) {
    int err;

    if (wait_queue == NULL) {
        wait_queue = global_sleep_queue;
    }

    if ((err = sched_wait_validate_input(wait_queue, state))) {
        return err;
    }

    queue_lock(wait_queue);
    current_lock();

    if ((err = current_interrupted(NULL))) {
        queue_unlock(wait_queue);
        current_unlock();
        return err;
    }

    // Insert the current thread into the wait queue.
    if ((err = embedded_enqueue_whence(wait_queue, &current->t_wait_qnode, QUEUE_UNIQUE, whence))) {
        current_unlock();
        queue_unlock(wait_queue);
        return err;
    }

    queue_unlock(wait_queue); // relinquish wait_queue lock.

    // set the wait queue back pointer.
    current->t_wait_queue = wait_queue;

    current_enter_state(state); // enter the respective waiting state.

    sched_block(lock);

    err = current_interrupted(NULL);
    
    current_unlock();
    return err;
}

int sched_wait(queue_t *wait_queue, tstate_t state, spinlock_t *lock) {
    return sched_wait_whence(wait_queue, state, QUEUE_TAIL, lock);
}

static int sched_wake_thread(thread_t *thread, wakeup_t reason) {
    if (thread == NULL || !wakeup_reason_validate(reason)) {
        return -EINVAL;
    }

    thread_assert_locked(thread);

    if (!thread_in_state(thread, T_STOPPED) && !thread_in_state(thread, T_SLEEP)) {
        debug("Thread[%d:%d]: Neither is sleeping nor stopped but has a wait queue.\n",
            thread_getpid(thread), thread_gettid(thread));
        return -EINVAL;
    }

    if (thread_ispark(thread)) {
        thread_setwake(thread);
        thread_mask_park(thread);
    }

    thread->t_wakeup = reason;

    return thread_schedule(thread);
}

int sched_detach_and_wakeup(queue_t *wait_queue, thread_t *thread, wakeup_t reason) {
    int err;

    if (wait_queue == NULL || thread == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(wait_queue);
    thread_assert_locked(thread);

    if ((err = embedded_queue_detach(wait_queue, &thread->t_wait_qnode))) {
        return err;
    }

    if ((err = sched_wake_thread(thread, reason))) {
        return err;
    }

    // reset the wait queue's back pointer to NULL.
    thread->t_wait_queue = NULL;

    return 0;
}

int sched_wakeup_whence(queue_t *wait_queue, wakeup_t reason, queue_relloc_t whence) {
    int err;
    thread_t *thread;

    if (wait_queue == NULL) {
        return -EINVAL;
    }

    queue_lock(wait_queue);

    switch (whence) {
        case QUEUE_TAIL: // retrieve a thread from the back of the queue.
            // wakeup a thread at the back of the queue.
            foreach_thread_reverse(wait_queue, thread, t_wait_qnode) {
                thread_lock(thread);

                err = sched_detach_and_wakeup(wait_queue, thread, reason);
                thread_unlock(thread);
                queue_unlock(wait_queue);
                return err;
            }

            break;

        case QUEUE_HEAD: // retrieve a thread from the front of the queue.
            foreach_thread(wait_queue, thread, t_wait_qnode) {
                thread_lock(thread);

                err = sched_detach_and_wakeup(wait_queue, thread, reason);
                thread_unlock(thread);
                queue_unlock(wait_queue);
                return err;
            }

            break;

        default:
            queue_unlock(wait_queue);
            return -EINVAL;
    }

    queue_unlock(wait_queue);
    return -ESRCH;  // No thread was waiting on this wait queue.
}

int sched_wakeup(queue_t *wait_queue, wakeup_t reason) {
    return sched_wakeup_whence(wait_queue, reason, QUEUE_HEAD);
}

int sched_wakeup_specific(queue_t *wait_queue, wakeup_t reason, tid_t tid) {
    if (wait_queue == NULL || tid <= 0) {
        return -EINVAL;
    }

    queue_lock(wait_queue);

    thread_t *thread;
    // retrieve a thread from the front of the queue.
    foreach_thread(wait_queue, thread, t_wait_qnode) {
        thread_lock(thread);

        if (thread_gettid(thread) == tid) {
            int err = sched_detach_and_wakeup(wait_queue, thread, reason);
            thread_unlock(thread);
            queue_unlock(wait_queue);
            return err;
        }

        thread_unlock(thread);
    }

    queue_unlock(wait_queue);
    return -ESRCH;  // No thread was waiting on this wait queue.
}

int sched_wakeup_all(queue_t *wait_queue, wakeup_t reason, size_t *pnt) {
    int      err     = 0;
    size_t   count   = 0;

    if (wait_queue == NULL) {
        return -EINVAL;
    }

    queue_lock(wait_queue);

    thread_t *thread;
    queue_foreach_entry(wait_queue, thread, t_wait_qnode) {
        thread_lock(thread);
        if ((err = sched_detach_and_wakeup(wait_queue, thread, reason))) {
            thread_unlock(thread);
            queue_unlock(wait_queue);
             if (pnt) *pnt = count;
            return err;
        }

        thread_unlock(thread);
        count += 1;
    }

    queue_unlock(wait_queue);

    if (pnt) *pnt = count;

    return count > 0 ? 0 : -ESRCH;
}

usize sched_wait_queue_length(queue_t *wait_queue) {
    queue_lock(wait_queue);
    const usize length = queue_length(wait_queue);
    queue_unlock(wait_queue);
    return length;
}