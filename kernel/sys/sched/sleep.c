#include <bits/errno.h>
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

    if ((err = embedded_enqueue_whence(wait_queue, &current->t_wait_qnode, QUEUE_ENFORCE_UNIQUE, whence))) {
        current_unlock();
        queue_unlock(wait_queue);
        return err;
    }
    queue_unlock(wait_queue);

    // set the wait queue back pointer.
    current->t_wait_queue = wait_queue;

    current_enter_state(state);

    if (lock) spin_unlock(lock);

    sched();

    if (lock) spin_lock(lock);

    if (current_iskilled()) {
        current_unlock();
        return -EINTR;
    }

    current_unlock();
    return 0;
}

static int sched_wake_thread(thread_t *thread) {
    if (thread == NULL)
        return -EINVAL;

    thread_assert_locked(thread);

    if (!thread_in_state(thread, T_STOPPED) && !thread_in_state(thread, T_SLEEP)) {
        debug("Thread[%d:%d]: Neither is sleeping nor stopped but has a wait queue.\n",
            thread_getpid(thread), thread_gettid(thread));
        return -EINVAL;
    }

    if (thread_ispark(thread))
        thread_setwake(thread);

    return thread_schedule(thread);
}

int sched_detach_and_wakeup(queue_t *wait_queue, thread_t *thread) {
    int err = 0;

    if (wait_queue == NULL || thread == NULL)
        return -EINVAL;
    
    queue_assert_locked(wait_queue);
    thread_assert_locked(thread);

    if ((err = embedded_queue_detach(wait_queue, &thread->t_wait_qnode)))
        return err;

    if ((err = sched_wake_thread(thread)))
        return err;

    // reset the wait queue back pointer to NULL.
    thread->t_wait_queue = NULL;

    return 0;
}

int sched_wakeup(queue_t *wait_queue, queue_relloc_t whence) {
    int      err = 0;

    if (wait_queue == NULL)
        return -EINVAL;

    queue_lock(wait_queue);

    if (whence == QUEUE_RELLOC_TAIL) { // retrieve a thread from the back of the queue.
        goto from_tail;
    } else if (whence != QUEUE_RELLOC_HEAD) {
        queue_unlock(wait_queue);
        return -EINVAL;
    }

    // retrieve a thread from the front of the queue.
    embedded_queue_foreach(wait_queue, thread_t, thread, t_wait_qnode) {
        thread_lock(thread);
        
        err = sched_detach_and_wakeup(wait_queue, thread);
        thread_unlock(thread);
        queue_unlock(wait_queue);
        return err;
    }

    goto done; // done here.
from_tail: // wakeup a thread at the back of the queue.
    embedded_queue_foreach_reverse(wait_queue, thread_t, thread, t_wait_qnode) {
        thread_lock(thread);
        
        err = sched_detach_and_wakeup(wait_queue, thread);
        thread_unlock(thread);
        queue_unlock(wait_queue);
        return err;
    }

done:
    queue_unlock(wait_queue);
    return -ESRCH;  // No thread was waiting on this wait queue.
}

int sched_wakeup_specific(queue_t *wait_queue, tid_t tid) {
    int      err = 0;

    if (wait_queue == NULL || tid <= 0)
        return -EINVAL;

    queue_lock(wait_queue);

    // retrieve a thread from the front of the queue.
    embedded_queue_foreach(wait_queue, thread_t, thread, t_wait_qnode) {
        thread_lock(thread);

        if (thread_gettid(thread) == tid) {
            err = sched_detach_and_wakeup(wait_queue, thread);
            thread_unlock(thread);
            queue_unlock(wait_queue);
            return err;
        }

        thread_unlock(thread);
    }

    queue_unlock(wait_queue);
    return -ESRCH;  // No thread was waiting on this wait queue.
}

int sched_wakeup_all(queue_t *wait_queue, size_t *pnt) {
    int      err     = 0;
    size_t   count   = 0;

    if (wait_queue == NULL)
        return -EINVAL;

    queue_lock(wait_queue);

    embedded_queue_foreach(wait_queue, thread_t, thread, t_wait_qnode) {
        thread_lock(thread);

        if ((err = sched_detach_and_wakeup(wait_queue, thread))) {
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