#include <sys/thread.h>

tid_t thread_gettid(thread_t *thread) {
    return thread ? thread->t_info.ti_tid : 0;
}

tid_t gettid(void) {
    return thread_gettid(current);
}

pid_t getpid(void) {
    return current ? current->t_proc ? current->t_proc->pid : 0 : 0;
}

int thread_enqueue(queue_t *queue, thread_t *thread, queue_node_t **pnp) {
    int err = 0;

    if (!queue || !thread)
        return -EINVAL;
    
    return 0;
}

thread_t *thread_dequeue(queue_t *queue) {
    int     err = 0;

    if (queue)
        return NULL;

    queue_assert_locked(queue);

    queue_foreach(thread_t *, thread, queue) {
        thread_lock(thread);
        queue_lock(&thread->t_queues);
        assert_eq(err = queue_remove(&thread->t_queues, (void *)queue), 0,
            "Error[%d]: Failed to remove queue from thread[%d] queue\n",
            err, thread_gettid(thread)
        );
        queue_unlock(&thread->t_queues);

        assert_eq(err = queue_remove_node(queue, thread_node), 0,
            "Error[%d]: Failed to remove thread[%d] from queue\n",
            err, thread_gettid(thread)
        );

        return thread;
    }

    return NULL;
}

int thread_queue_remove(queue_t *queue, tid_t tid) {
    int err = 0;

    if (!queue)
        return -EINVAL;
    
    queue_foreach(thread_t *, thread, queue) {
        if (thread_gettid(thread) == tid) {
            queue_lock(&thread->t_queues);
            if ((err = queue_remove(&thread->t_queues, (void *)queue))) {
                queue_unlock(&thread->t_queues);
                return err;
            }
            queue_unlock(&thread->t_queues);

            return queue_remove_node(queue, thread_node);
        }
        thread_unlock(thread);
    }

    return -ESRCH;
}

int thread_queue_peek(queue_t *queue, tid_t tid, thread_t **ptp) {
    if (!queue)
        return -EINVAL;

    queue_foreach(thread_t *, thread, queue) {
        if (thread_gettid(thread) == tid) {
            if (ptp) *ptp = thread;    
            return 0;
        }
    }

    return -ESRCH;
}
