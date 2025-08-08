#include <bits/errno.h>
#include <sys/schedule.h>
#include <sys/thread.h>

/**
 * @brief check the reason for the interrupt.
 * 
 * @param reason[in] is one of four(4) values as specified by 'wakeup_t' enum.
 * @return true if reason is != WAKEUP_NONE or != WAKEUP_NORMAL.
 * @return false if reason is WAKEUP_SIGNAL or WAKEUP_TIMEOUT.
 */
static bool wakeup_interrupt(wakeup_t reason) {
    return ((reason == WAKEUP_SIGNAL) || (reason == WAKEUP_TIMEOUT)) ? true : false;
}

/**
 * @brief 
 * 
 * @param preason 
 * @return int 
 */
int current_interrupted(wakeup_t *preason) {
    wakeup_t reason = WAKEUP_NONE;

    current_assert_locked();
    
    if (current->t_wakeup != WAKEUP_NONE) {
        reason = current->t_wakeup;
        current->t_wakeup = WAKEUP_NONE;

        if (reason == WAKEUP_SIGNAL) {
            signal_check_pending();
        }
    }
    
    if (preason) *preason = reason;

    if (current_is_canceled() || wakeup_interrupt(reason)) {
        return reason == WAKEUP_TIMEOUT ? -ETIMEDOUT : -EINTR;
    }

    return 0;
}

int thread_wakeup(thread_t *thread, wakeup_t reason) {    
    if (thread == NULL || !validate_wakeup_reason(reason)) {
        return -EINVAL;
    }

    thread_assert_locked(thread);

    if (thread_isblocked(thread)) {
        if (thread->t_wait_queue) { // thread is blocked and is on a wait queue?
            queue_t *wait_queue = thread->t_wait_queue;
            queue_lock(wait_queue);
            int err = sched_detach_and_wakeup(wait_queue, thread, reason);
            queue_unlock(wait_queue);
            return err;
        }

        // thread is blocked and is not on a wait queue.

        thread->t_wakeup = reason;
        return thread_schedule(thread);
    }

    return 0;
}

int thread_sleep(wakeup_t *preason) {
    int err;

    current_assert_locked();

    if ((err = current_interrupted(preason))) {
        return err;
    }
    
    current_enter_state(T_SLEEP);
    sched();
    
    if ((err = current_interrupted(preason))) {
        return err;
    }

    return err;
}


int thread_wait(thread_t *thread) {
    if (thread == NULL) {
        return -EINVAL;
    }

    if (thread == current) {
        return -EDEADLK;
    }

    if (thread_iszombie(thread) || thread_isterminated(thread)) {
        return 0;
    }

    while (!thread_in_state(thread, T_ZOMBIE)) {
        int err = cond_wait_releasing(&thread->t_event, &thread->t_lock);
        if (err != 0) {
            return err;
        }
    }

    return 0;
}

int thread_join(tid_t tid, thread_info_t *info, void **prp) {
    int         err;
    thread_t    *thread = NULL;

    if (tid == gettid()) {
        return -EDEADLK;
    }

    if ((err = thread_group_get_by_tid(tid, &thread))) {
        return err;
    }

    // If we get here, target thread is a T_ZOMBIE and we can clean it.
    return thread_reap(thread, info, prp);
}