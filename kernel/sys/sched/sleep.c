#include <bits/errno.h>
#include <sys/schedule.h>
#include <sys/thread.h>

void sched_yield(void) {
    current_lock();
    sched();
    current_unlock();
}

int sched_wait(thread_queue_t *wait_queue, tstate_t state, queue_relloc_t whence, spinlock_t *lock) {
    int err = 0;

    if (wait_queue == NULL || current == NULL)
        return -EINVAL;

    thread_queue_lock(wait_queue);
    queue_lock(&wait_queue->t_queue);
    current_lock();

    if ((err = embedded_enqueue_whence(&wait_queue->t_queue, &current->t_wait_qnode, QUEUE_ENFORCE_UNIQUE, whence))) {
        current_unlock();
        queue_unlock(&wait_queue->t_queue);
        thread_queue_unlock(wait_queue);
        return err;
    }
    queue_unlock(&wait_queue->t_queue);
    thread_queue_unlock(wait_queue);

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

    if (thread_iszombie(thread) || thread_isterminated(thread))
        return 0;

    if (thread_ispark(thread))
        thread_setwake(thread);

    return thread_schedule(thread);
}

int sched_wakeup(thread_queue_t *wait_queue, queue_relloc_t whence) {
    int      err = 0;

    if (wait_queue == NULL)
        return -EINVAL;

    thread_queue_lock(wait_queue);
    queue_lock(&wait_queue->t_queue);

    if (whence == QUEUE_RELLOC_TAIL) { // retrieve a thread from the back of the queue.
        goto from_tail;
    } else if (whence != QUEUE_RELLOC_HEAD) {
        queue_unlock(&wait_queue->t_queue);
        thread_queue_unlock(wait_queue);
        return -EINVAL;
    }

    // retrieve a thread from the front of the queue.
    embedded_queue_foreach(&wait_queue->t_queue, thread_t, thread, t_wait_qnode) {
        thread_lock(thread);

        if ((err = embedded_queue_detach(&wait_queue->t_queue, thread_node))) {
            thread_unlock(thread);
            queue_unlock(&wait_queue->t_queue);
            thread_queue_unlock(wait_queue);
            return err;
        }

        queue_unlock(&wait_queue->t_queue);
        thread_queue_unlock(wait_queue);

        if ((err = sched_wake_thread(thread))) {
            thread_unlock(thread);
            return err;
        }

        thread_unlock(thread);

        return 0;
    }

    goto done; // done here.
from_tail: // wakeup a thread at the back of the queue.
    embedded_queue_foreach_reverse(&wait_queue->t_queue, thread_t, thread, t_wait_qnode) {
        thread_lock(thread);

        if ((err = embedded_queue_detach(&wait_queue->t_queue, thread_node))) {
            thread_unlock(thread);
            queue_unlock(&wait_queue->t_queue);
            thread_queue_unlock(wait_queue);
            return err;
        }

        queue_unlock(&wait_queue->t_queue);
        thread_queue_unlock(wait_queue);

        if ((err = sched_wake_thread(thread))) {
            thread_unlock(thread);
            return err;
        }

        thread_unlock(thread);

        return 0;
    }

done:
    queue_unlock(&wait_queue->t_queue);
    thread_queue_unlock(wait_queue);
    return -ESRCH;  // No thread was waiting on this wait queue.
}

int sched_wakeup_all(thread_queue_t *wait_queue, size_t *pnt) {
    int      err     = 0;
    size_t   count   = 0;

    if (wait_queue == NULL)
        return -EINVAL;

    thread_queue_lock(wait_queue);

    queue_lock(&wait_queue->t_queue);

    embedded_queue_foreach(&wait_queue->t_queue, thread_t, thread, t_wait_qnode) {
        thread_lock(thread);

        if ((err = embedded_queue_detach(&wait_queue->t_queue, thread_node))) {
            thread_unlock(thread);
            queue_unlock(&wait_queue->t_queue);
            thread_queue_unlock(wait_queue);
            if (pnt) *pnt = count;
            return err;
        }

        if ((err = sched_wake_thread(thread))) {
            thread_unlock(thread);
            queue_unlock(&wait_queue->t_queue);
            thread_queue_unlock(wait_queue);
            if (pnt) *pnt = count;
            return err;
        }

        thread_unlock(thread);

        count += 1;
    }

    queue_unlock(&wait_queue->t_queue);
    thread_queue_unlock(wait_queue);

    if (pnt) *pnt = count;

    return count > 0 ? 0 : -ESRCH;
}