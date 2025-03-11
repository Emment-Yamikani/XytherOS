#include <bits/errno.h>
#include <core/debug.h>
#include <sys/schedule.h>
#include <sys/thread.h>

void sched_yield(void) {
    current_lock();
    current_enter_state(T_READY);
    sched();
    current_unlock();
}

int sched_wait(queue_t *wait_queue, tstate_t state, queue_relloc_t whence, spinlock_t *lock) {
    int err = 0;

    if (wait_queue == NULL || current == NULL)
        return -EINVAL;
    
    // Thread can only enter a wait state as SLEEPING or STOPPED.
    if (state != T_SLEEP && state != T_STOPPED)
        return -EINVAL;

    queue_lock(wait_queue);
    current_lock();

    // Insert the current thread into the wait queue.
    if ((err = embedded_enqueue_whence(wait_queue, &current->t_wait_qnode, QUEUE_UNIQUE, whence))) {
        current_unlock();
        queue_unlock(wait_queue);
        return err;
    }

    queue_unlock(wait_queue);   // relinquish wait_queue lock.

    // set the wait queue back pointer.
    current->t_wait_queue = wait_queue;

    current_enter_state(state); // enter the respective waiting state.

    if (lock) spin_unlock(lock); // releasse the lock.

    sched();

    if (lock) spin_lock(lock);   // re-acquire the lock.

    // check if current thread was sent cancelation request.
    if (current_iscanceled() || current->t_wakeup_reason == WAKEUP_SIGNAL) {
        current_unlock();
        return -EINTR; // thread was sent a cancelation request.
    }

    current_unlock();
    return 0;
}

static int sched_wake_thread(thread_t *thread, wakeup_reason_t reason) {
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

    thread->t_wakeup_reason = reason;

    return thread_schedule(thread);
}

int sched_detach_and_wakeup(queue_t *wait_queue, wakeup_reason_t reason, thread_t *thread) {
    int err;

    if (wait_queue == NULL || thread == NULL)
        return -EINVAL;
    
    queue_assert_locked(wait_queue);
    thread_assert_locked(thread);

    if ((err = embedded_queue_detach(wait_queue, &thread->t_wait_qnode)))
        return err;

    if ((err = sched_wake_thread(thread, reason)))
        return err;

    // reset the wait queue's back pointer to NULL.
    thread->t_wait_queue = NULL;

    return 0;
}

int sched_wakeup(queue_t *wait_queue, wakeup_reason_t reason, queue_relloc_t whence) {
    int err;

    if (wait_queue == NULL)
        return -EINVAL;

    queue_lock(wait_queue);

    switch (whence) {
        case QUEUE_TAIL: // retrieve a thread from the back of the queue.
            // wakeup a thread at the back of the queue.
            embedded_queue_foreach_reverse(wait_queue, thread_t, thread, t_wait_qnode) {
                thread_lock(thread);

                err = sched_detach_and_wakeup(wait_queue, reason, thread);
                thread_unlock(thread);
                queue_unlock(wait_queue);
                return err;
            }

            break;

        case QUEUE_HEAD: // retrieve a thread from the front of the queue.
            embedded_queue_foreach(wait_queue, thread_t, thread, t_wait_qnode) {
                thread_lock(thread);
                
                err = sched_detach_and_wakeup(wait_queue, reason, thread);
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

int sched_wakeup_specific(queue_t *wait_queue, wakeup_reason_t reason, tid_t tid) {
    
    if (wait_queue == NULL || tid <= 0)
    return -EINVAL;
    
    queue_lock(wait_queue);
    
    // retrieve a thread from the front of the queue.
    embedded_queue_foreach(wait_queue, thread_t, thread, t_wait_qnode) {
        thread_lock(thread);
        
        if (thread_gettid(thread) == tid) {
            int err = sched_detach_and_wakeup(wait_queue, reason, thread);
            thread_unlock(thread);
            queue_unlock(wait_queue);
            return err;
        }

        thread_unlock(thread);
    }

    queue_unlock(wait_queue);
    return -ESRCH;  // No thread was waiting on this wait queue.
}

int sched_wakeup_all(queue_t *wait_queue, wakeup_reason_t reason, size_t *pnt) {
    int      err     = 0;
    size_t   count   = 0;

    if (wait_queue == NULL)
        return -EINVAL;

    queue_lock(wait_queue);

    embedded_queue_foreach(wait_queue, thread_t, thread, t_wait_qnode) {
        thread_lock(thread);

        if ((err = sched_detach_and_wakeup(wait_queue, reason, thread))) {
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