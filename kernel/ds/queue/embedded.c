#include <bits/errno.h>
#include <ds/queue.h>

void embedded_queue_flush(queue_t *queue) {
    queue_node_t *next = NULL, *prev = NULL;

    queue_assert_locked(queue);

    forlinked(node, queue->head, next) {
        next = node->next;
        prev = node->prev;

        if (prev)
            prev->next = next;
        if (next)
            next->prev = prev;
        if (node == queue->head)
            queue->head = next;
        if (node == queue->tail)
            queue->tail = prev;

        queue->q_count--;

        node->next  = NULL;
        node->prev  = NULL;
    }

    queue->head = NULL;
    queue->tail = NULL;
}

int embedded_queue_peek(queue_t *queue, queue_relloc_t whence, queue_node_t **pnp) {
    if (queue == NULL || pnp == NULL)
        return -EINVAL;

    if (whence != QUEUE_RELLOC_HEAD && whence != QUEUE_RELLOC_TAIL)
        return -EINVAL;

    queue_assert_locked(queue);

    if (queue_count(queue) == 0)
        return -ENOENT;

    if (whence == QUEUE_RELLOC_TAIL)
        *pnp = queue->tail;
    else *pnp = queue->head;
    return 0;
}

int embedded_queue_contains(queue_t *queue, queue_node_t *qnode) {
    if (queue == NULL || qnode == NULL)
        return -EINVAL;

    queue_assert_locked(queue);

    forlinked(node, queue->head, node->next) {
        if (node == qnode)
            return 0;
    }

    return -ENOENT;
}

int embedded_enqueue(queue_t *queue, queue_node_t *qnode, queue_uniqueness_t uniqueness) {
    if (queue == NULL || qnode == NULL)
        return -EINVAL;

    queue_assert_locked(queue);

    if (uniqueness == QUEUE_ENFORCE_UNIQUE){
        if (embedded_queue_contains(queue, qnode) == 0)
            return -EEXIST;
    }

    qnode->prev = qnode->next = NULL;

    if (queue->head == NULL) {
        queue->tail = queue->head = qnode;
    } else {
        queue->tail->next = qnode;
        qnode->prev = queue->tail;
    }

    queue->tail  = qnode;

    queue->q_count++;
    return 0;
}

int embedded_enqueue_head(queue_t *queue, queue_node_t *qnode, queue_uniqueness_t uniqueness) {
    if (queue == NULL || qnode == NULL)
        return -EINVAL;

    queue_assert_locked(queue);

    if (uniqueness == QUEUE_ENFORCE_UNIQUE) {
        if (embedded_queue_contains(queue, qnode) == 0)
            return -EEXIST;
    }

    qnode->prev = qnode->next = NULL;

    if (queue->head == NULL) {
        queue->tail = queue->head = qnode;
    } else {
        queue->head->prev = qnode;
        qnode->next = queue->head;
    }

    queue->head  = qnode;

    queue->q_count++;
    return 0;
}

int embedded_enqueue_whence(queue_t *queue, queue_node_t *qnode, queue_uniqueness_t uniqueness, queue_relloc_t whence) {
    queue_assert_locked(queue);
    if (whence == QUEUE_RELLOC_HEAD)
        return embedded_enqueue_head(queue, qnode, uniqueness);
    else if (whence == QUEUE_RELLOC_TAIL)
        return embedded_enqueue(queue, qnode, uniqueness);
    return -EINVAL;
}

int embedded_dequeue(queue_t *queue, queue_node_t **pnp) {
    queue_node_t *node = NULL, *prev = NULL, *next = NULL;

    if (queue == NULL || pnp == NULL)
        return -EINVAL;

    queue_assert_locked(queue);

    node = queue->head;
    if (node) {
        prev = node->prev;
        next = node->next;

        if (prev)
            prev->next = next;
        if (next)
            next->prev = prev;
        if (node == queue->head)
            queue->head = next;
        if (node == queue->tail)
            queue->tail = prev;

        queue->q_count--;

        node->next  = NULL;
        node->prev  = NULL;

        *pnp = node;
        return 0;
    }

    return -ENOENT;
}

int embedded_dequeue_tail(queue_t *queue, queue_node_t **pnp) {
    queue_node_t *next = NULL, *node = NULL, *prev = NULL;

    if (queue == NULL || pnp == NULL)
        return -EINVAL;

    queue_assert_locked(queue);

    node = queue->tail;
    
    if (node) {
        next = node->next;
        prev = node->prev;

        if (prev)
            prev->next = next;
        if (next)
            next->prev = prev;
        if (node == queue->head)
            queue->head = next;
        if (node == queue->tail)
            queue->tail = prev;

        queue->q_count--;

        node->next  = NULL;
        node->prev  = NULL;

        *pnp = node;
        return 0;
    }

    return -ENOENT;
}

int embedded_dequeue_whence(queue_t *queue, queue_relloc_t whence, queue_node_t **pnp) {
    queue_assert_locked(queue);
    if (whence == QUEUE_RELLOC_HEAD)
        return embedded_dequeue(queue, pnp);
    else if (whence == QUEUE_RELLOC_TAIL)
        return embedded_dequeue_tail(queue, pnp);
    return -EINVAL;
}

int embedded_queue_detach(queue_t *queue, queue_node_t *qnode) {
    queue_node_t *next = NULL, *prev = NULL;

    if (queue == NULL || qnode == NULL)
        return -EINVAL;

    queue_assert_locked(queue);

    next = qnode->next;
    prev = qnode->prev;

    if (prev)
        prev->next = next;

    if (next)
        next->prev = prev;

    if (qnode == queue->head)
        queue->head = next;

    if (qnode == queue->tail)
        queue->tail = prev;

    queue->q_count--;

    qnode->next  = NULL;
    qnode->prev  = NULL;

    return 0;
}

int embedded_queue_remove(queue_t *queue, queue_node_t *qnode) {
    int err = 0;

    if (queue == NULL || qnode == NULL)
        return -EINVAL;

    queue_assert_locked(queue);

    if ((err = embedded_queue_contains(queue, qnode)))
        return err;

    return embedded_queue_detach(queue, qnode);
}

int embedded_queue_replace(queue_t *queue, queue_node_t *qnode0, queue_node_t *qnode1) {
    if (!queue || qnode0 == NULL || qnode1 == NULL)
        return -EINVAL; // Invalid arguments.

    queue_assert_locked(queue);

    if (qnode0 == qnode1)
        return 0;

    // Iterate through the queue to find the node containing `qnode0`.
    forlinked(node, queue->head, node->next) {
        if (node == qnode0) {
            qnode1->next = node->next;
            qnode1->prev = node->prev;

            if (qnode1->next) {
                qnode1->next->prev = qnode1;
            }

            if (qnode1->prev) {
                qnode1->prev->next = qnode1;
            }

            if (queue->head == node) {
                queue->head = qnode1;
            }

            if (queue->tail == node) {
                queue->tail = qnode1;
            }

            node->next = NULL;
            node->prev = NULL;
            return 0;
        }
    }

    // If we reach here, `qnode0` was not found.
    return -ENOENT; // No such entry.
}

int embedded_queue_migrate(queue_t *dst, queue_t *src, usize pos, usize n, queue_relloc_t whence) {
    usize        index = 0;
    queue_node_t *node = NULL;

    if (!dst || !src)
        return -EINVAL;

    queue_assert_locked(dst);
    queue_assert_locked(src);

    if ((pos >= src->q_count) || (n == 0) || ((pos + n) > src->q_count))
        return -EINVAL;

    if (whence != QUEUE_RELLOC_HEAD && whence != QUEUE_RELLOC_TAIL)
        return -EINVAL; // Error: invalid rellocation position

    // Traverse src queue to the starting position.
    for (node = src->head; node && index < pos; ++index) {
        node = node->next;
    }

    if (node == NULL)
        return -ENOENT; // Error: starting position not found.

    queue_node_t *last  = node;
    queue_node_t *first = node;

    // Find the ending node.
    for (usize i = 1; i < n; ++i) {
        last = last->next;
    }

    // Remove the nodes from source queue.
    if (first->prev)
        first->prev->next = last->next;
    else src->head = last->next;

    if (last->next)
        last->next->prev = first->prev;
    else src->tail = first->prev;

    src->q_count -= n; // Update source queue item count.

    first->prev = NULL;
    last->next  = NULL;

    // Attach nodes to the destination queue.
    if (whence == QUEUE_RELLOC_HEAD) {
        last->next = dst->head;
        
        if (dst->head)
            dst->head->prev = last;
        
        dst->head = first;
        
        if (dst->tail == NULL)
            dst->tail = last;
    } else if (whence == QUEUE_RELLOC_TAIL) {
        first->prev = dst->tail;
        if (dst->tail)
            dst->tail->next = first;
        
        dst->tail = last;
        
        if (dst->head == NULL)
            dst->head = first;
    }

    dst->q_count += n; // Update destination queue item count.
    return 0;
}