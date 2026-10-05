#include <bits/errno.h>
#include <ds/queue.h>
#include <mm/kalloc.h>
#include <core/debug.h>

static int at_display(queue_t *queue, qnode_t *node, qnode_into_iter_fn_t) {
    printk("%ld%s", (long)node->data, node->next != &queue->q_sentinel ? ", " : "");
    return 0;
}

void embedded_queue_display(queue_t *queue) {
    bool locked = queue_recursive_lock(queue);

    printk("Queue(length: %3ld): [", queue->q_count);
    embedded_queue_into_iter(queue, at_display, NULL);
    printk("]\n");
    locked ? queue_unlock(queue) : 0;
}

bool embedded_queue_empty(queue_t *queue) {
    queue_assert_locked(queue);
    return &queue->q_sentinel == queue->q_sentinel.next;
}

int embedded_queue_init(queue_t *queue) {
    if (queue == NULL) {
        return -EINVAL;
    }

    spinlock_init(&queue->q_lock);

    queue->q_count      = 0;
    queue->q_sentinel   = QNODE_INIT(queue->q_sentinel, queue);

    return 0;
}

int embedded_queue_create(queue_t **pqueue) {
    if (pqueue == NULL) {
        return -EINVAL;
    }

    queue_t *queue = (queue_t *)kzalloc(sizeof *queue);
    if (queue == NULL) {
        return -ENOMEM;
    }

    int err = embedded_queue_init(queue);
    if (err) {
        kfree(queue);
        return err;
    }

    *pqueue = queue;

    return 0;
}

int embedded_queue_into_iter(queue_t *queue, embedded_queue_into_iter_fn_t queue_into_iter, qnode_into_iter_fn_t qnode_into_iter) {
    if (queue == NULL || queue_into_iter == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    qnode_t *node, *next;
    qnode_foreach_node_safe(&queue->q_sentinel, node, next) {
        int err = queue_into_iter(queue, node, qnode_into_iter);
        if (err) { return err; }
    }

    return 0;
}

static int at_drain(queue_t *queue, qnode_t *node, qnode_into_iter_fn_t qnode_into_iter) {
    qnode_t *prev = node->prev, *next = node->next;
    int err = qnode_detach(node);
    if (err) { return err; }

    if (qnode_into_iter) {
        if ((err = qnode_into_iter(node))) {
            node->prev = prev;
            node->next = next;
    
            prev->next = node;
            next->prev = node;
            return err;
        }
    }

    queue->q_count  -= 1;
    return 0;
}

int embedded_queue_drain(queue_t *queue, qnode_into_iter_fn_t qnode_at_drain) {
    queue_assert_locked(queue);
    return embedded_queue_into_iter(queue, at_drain, qnode_at_drain);
}

void embedded_queue_flush(queue_t *queue) {
    queue_assert_locked(queue);
    int err = embedded_queue_drain(queue, NULL);
    assert_eq(err, 0, "Failed to drain queue\n");
}

void embedded_queue_destroy(queue_t *queue, qnode_into_iter_fn_t node_at_destroy) {
    queue_recursive_lock(queue);
    int err = embedded_queue_drain(queue, node_at_destroy);
    assert_eq(err, 0, "Failed to drain the queue\n");
    queue_unlock(queue);
    kfree(queue);
}

size_t embedded_queue_length(queue_t *queue) {
    queue_assert_locked(queue);
    return queue->q_count;
}

void embedded_queue_print(queue_t *queue) {
    queue_assert_locked(queue);

    printk("Queue (length: %ld nodes): ", embedded_queue_length(queue));
    qnode_print(&queue->q_sentinel);
}

int embedded_queue_peek(queue_t *queue, queue_relloc_t whence, qnode_t **pnp) {
    if (queue == NULL || pnp == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    if (embedded_queue_empty(queue)) {
        return -ENOENT;
    }

    *pnp = whence == QUEUE_TAIL ? queue->q_sentinel.prev : queue->q_sentinel.next;

    return 0;
}

int embedded_queue_contains(queue_t *queue, qnode_t *qnode) {
    if (queue == NULL || qnode == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    return qnode_contains(&queue->q_sentinel, qnode);
}

int embedded_dequeue_whence(queue_t *queue, queue_relloc_t whence, qnode_t **pnp) {
    if (queue == NULL || pnp == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    if (embedded_queue_empty(queue)) {
        return -ENOENT;
    }

    int err;
    qnode_t *node = whence == QUEUE_TAIL ? queue->q_sentinel.prev : queue->q_sentinel.next;
    if ((err = qnode_detach(node))) {
        return err;
    }

    queue->q_count -= 1;

    *pnp = node;
    return 0;
}

int embedded_dequeue(queue_t *queue, qnode_t **pnp) {
    if (queue == NULL || pnp == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    return embedded_dequeue_whence(queue, QUEUE_HEAD, pnp);
}

int embedded_dequeue_tail(queue_t *queue, qnode_t **pnp) {
    if (queue == NULL || pnp == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    return embedded_dequeue_whence(queue, QUEUE_TAIL, pnp);
}

int embedded_enqueue_whence(queue_t *queue, qnode_t *qnode, queue_uniqueness_t uniqueness, queue_relloc_t whence) {
    if (queue == NULL || qnode == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    int err = qnode_enqueue_whence(&queue->q_sentinel, qnode, uniqueness, whence);
    if (err) { return err; }

    queue->q_count += 1;

    return 0;
}

int embedded_enqueue(queue_t *queue, qnode_t *qnode, queue_uniqueness_t uniqueness) {
    if (queue == NULL || qnode == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    return embedded_enqueue_whence(queue, qnode, uniqueness, QUEUE_TAIL);
}

int embedded_enqueue_head(queue_t *queue, qnode_t *qnode, queue_uniqueness_t uniqueness) {
    if (queue == NULL || qnode == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    return embedded_enqueue_whence(queue, qnode, uniqueness, QUEUE_HEAD);
}

int embedded_enqueue_after(queue_t *queue, qnode_t *prev, qnode_t *node, queue_uniqueness_t uniqueness) {
    if (queue == NULL || node == NULL || prev == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    int err = qnode_contains(&queue->q_sentinel, prev);
    if (err) { return err; }

    if ((err = qnode_enqueue_after(prev, node, uniqueness))) {
        return err;
    }

    queue->q_count += 1;
    return 0;
}

int embedded_enqueue_before(queue_t *queue, qnode_t *next, qnode_t *qnode, queue_uniqueness_t uniqueness) {
     if (queue == NULL || qnode == NULL || next == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    int err = qnode_contains(&queue->q_sentinel, next);
    if (err) { return err; }

    if ((err = qnode_enqueue_before(next, qnode, uniqueness))) {
        return err;
    }

    queue->q_count += 1;
    return 0;
}

int embedded_queue_remove(queue_t *queue, qnode_t *qnode) {
    if (queue == NULL || qnode == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    int err = qnode_contains(&queue->q_sentinel, qnode);
    if (err) { return err; }

    if ((err = qnode_detach(qnode))) {
        return err;
    }

    queue->q_count -= 1;

    return 0;
}

int embedded_queue_replace(queue_t *queue, qnode_t *qnode0, qnode_t *qnode1) {
    if (queue == NULL || qnode0 == NULL || qnode1 == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    int err = qnode_contains(&queue->q_sentinel, qnode0);
    if (err) { return err; }

    if (qnode0 == qnode1) {
        return 0;
    }

    return qnode_replace(qnode0, qnode1);
}

int embedded_queue_migrate(queue_t *dst, queue_t *src, usize pos, usize n, queue_relloc_t whence) {
    if (dst == NULL || src == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(dst);
    queue_assert_locked(src);

    int err = qnode_migrate(&dst->q_sentinel, &src->q_sentinel, pos, n, whence);
    if (err) { return err; }

    dst->q_count += n;
    src->q_count -= n;

    return 0;
}

int embedded_queue_relloc(queue_t *dst, queue_t *src, qnode_t *node, queue_uniqueness_t uniqueness, queue_relloc_t whence) {
    if (dst == NULL || src == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(dst);
    queue_assert_locked(src);

    int err = embedded_queue_contains(src, node);
    if (err) { return err; }

    if ((err = qnode_relloc(&dst->q_sentinel, node, uniqueness, whence))) {
        return err;
    }

    src->q_count -= 1;
    dst->q_count += 1;

    return 0;
}

int embedded_sorted_enqueue(queue_t *queue, qnode_t *qnode, queue_uniqueness_t uniqueness, queue_order_t order, qnode_cmp_fn_t compare) {
    if (queue == NULL || qnode == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    int err = qnode_sorted_enqueue(&queue->q_sentinel, qnode, uniqueness, order, compare);
    if (err) { return err; }

    queue->q_count += 1;
    return 0;
}

int embedded_qnode_rotate_left(queue_t *queue) {
    if (queue == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    return qnode_rotate_left(&queue->q_sentinel);
}

int embedded_qnode_rotate_right(queue_t *queue) {
    if (queue == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    return qnode_rotate_right(&queue->q_sentinel);
}