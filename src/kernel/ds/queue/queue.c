#include <bits/errno.h>
#include <ds/queue.h>
#include <mm/kalloc.h>

void queue_display(queue_t *queue) {
    embedded_queue_display(queue);
}

int queue_init(queue_t *queue) {
    if (queue == NULL) {
        return -EINVAL;
    }

    spinlock_init(&queue->q_lock);

    queue->q_count      = 0;
    queue->q_sentinel   = QNODE_INIT(queue->q_sentinel, queue);

    return 0;
}

int queue_qnode_init(qnode_t *qnode, void *data) {
    return qnode_init(qnode, data);
}

int queue_create(queue_t **pqp) {
    return embedded_queue_create(pqp);
}

static int drop_node(qnode_t *node) {
    kfree(node);
    return 0;
}

void queue_destroy(queue_t *queue) {
    queue_recursive_lock(queue);
    embedded_queue_destroy(queue, drop_node);
}

void queue_flush(queue_t *queue) {
    queue_assert_locked(queue);
    int err = embedded_queue_drain(queue, drop_node);
    assert_eq(err, 0, "Failed to drain the queue\n");
}

size_t queue_length(queue_t *queue) {
    queue_assert_locked(queue);
    return embedded_queue_length(queue);
}

bool queue_empty(queue_t *queue) {
    queue_assert_locked(queue);
    return embedded_queue_empty(queue);
}

int queue_peek(queue_t *queue, queue_relloc_t whence, void **pdp) {
    if (queue == NULL || pdp == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    qnode_t *node;
    int err = embedded_queue_peek(queue, whence, &node);
    if (err) { return err; }

    *pdp = node->data;

    return 0;
}

int queue_contains(queue_t *queue, void *data, qnode_t **pnp) {
    if (queue == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    qnode_t *node;
    qnode_foreach_node(&queue->q_sentinel, node) {
        if (node->data == data) {
            if (pnp) { *pnp = node; }
            return 0;
        }
    }

    return -ENOENT;
}

int dequeue(queue_t *queue, void **pdp) {
    if (queue == NULL || pdp == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    return dequeue_whence(queue, QUEUE_HEAD, pdp);
}

int dequeue_tail(queue_t *queue, void **pdp) {
    if (queue == NULL || pdp == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    return dequeue_whence(queue, QUEUE_TAIL, pdp);
}

int dequeue_whence(queue_t *queue, queue_relloc_t whence, void **pdp) {
    if (queue == NULL || pdp == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    qnode_t *node;
    int err = embedded_dequeue_whence(queue, whence, &node);
    if (err) { return err; }

    *pdp = node->data;

    kfree(node);

    return 0;
}

int enqueue(queue_t *queue, void *data, queue_uniqueness_t uniqueness, qnode_t **pnp) {
    if (queue == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    return enqueue_whence(queue, data, uniqueness, QUEUE_TAIL, pnp);
}

int enqueue_head(queue_t *queue, void *data, queue_uniqueness_t uniqueness, qnode_t **pnp) {
    if (queue == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    return enqueue_whence(queue, data, uniqueness, QUEUE_HEAD, pnp);
}

int enqueue_whence(queue_t *queue, void *data, queue_uniqueness_t uniqueness, queue_relloc_t whence, qnode_t **pnp) {
    if (queue == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    qnode_t *node;
    int err = qnode_create(data, &node);
    if (err) { return err; }

    if ((err = embedded_enqueue_whence(queue, node, uniqueness, whence))) {
        qnode_destroy(node);
        return err;
    }

    if (pnp) *pnp = node;

    return 0;
}

int enqueue_sorted(queue_t *queue, void *data, queue_uniqueness_t uniqueness, queue_order_t order, qnode_cmp_fn_t compare, qnode_t **pnp) {
    if (queue == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    qnode_t *node;
    int err = qnode_create(data, &node);
    if (err) { return err; }

    if ((err = embedded_sorted_enqueue(queue, node, uniqueness, order, compare))) {
        qnode_destroy(node);
        return err;
    }

    if (pnp) { *pnp = node; }

    return 0;
}

int queue_remove(queue_t *queue, void *data) {
    if (queue == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    qnode_t *node, *next;
    qnode_foreach_node_safe(&queue->q_sentinel, node, next) {
        if (node->data == data) {
            qnode_destroy(node);
            queue->q_count -= 1;
            return 0;
        }
    }

    return -ENOENT;
}

int queue_remove_node(queue_t *queue, qnode_t *node) {
    if (queue == NULL || node == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    int err = embedded_queue_remove(queue, node);
    if (err) { return err; }

    qnode_destroy(node);
    return 0;
}

int queue_rellocate_node(queue_t *queue, qnode_t *node, queue_relloc_t whence) {
    if (queue == NULL || node == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    int err = embedded_queue_remove(queue, node);
    if (err) { return err; }

    return embedded_enqueue_whence(queue, node, QUEUE_UNIQUE, whence);
}

int queue_rellocate(queue_t *queue, void *data, queue_relloc_t whence) {
    if (queue == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    qnode_t *node;
    int err = queue_contains(queue, data, &node);
    if (err != 0) { return err; }

    return queue_rellocate_node(queue, node, whence);
}

int queue_qnode_migrate(queue_t *dstq, queue_t *srcq, usize start_pos, usize num_nodes, queue_relloc_t whence) {
    if (dstq == NULL || srcq == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(dstq);
    queue_assert_locked(srcq);

    return embedded_queue_migrate(dstq, srcq, start_pos, num_nodes, whence);
}

int queue_move(queue_t *dstq, queue_t *srcq, queue_relloc_t whence) {
    if (dstq == NULL || srcq == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(dstq);
    queue_assert_locked(srcq);

    return queue_qnode_migrate(dstq, srcq, 1, srcq->q_count, whence);
}

int queue_replace(queue_t *queue, void *data0, void *data1) {
    if (queue == NULL || data0 == data1) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    qnode_t *node;
    qnode_foreach_node(&queue->q_sentinel, node) {
        if (node->data == data0) {
            node->data = data1;
            return 0;
        }
    }

    return -ENOENT;
}

int queue_rotate_left(queue_t *queue) {
    if (queue == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    return embedded_qnode_rotate_left(queue);
}

int queue_rotate_right(queue_t *queue) {
    if (queue == NULL) {
        return -EINVAL;
    }

    queue_assert_locked(queue);
    return embedded_qnode_rotate_right(queue);
}