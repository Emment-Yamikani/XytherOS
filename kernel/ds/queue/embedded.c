#include <bits/errno.h>
#include <core/debug.h>
#include <ds/queue.h>

int validate_whence(queue_relloc_t whence) {
    return (whence != QUEUE_HEAD && whence != QUEUE_TAIL) ? -EINVAL : 0;
}

int validate_uniqueness(queue_uniqueness_t uniqueness) {
    return (uniqueness != QUEUE_UNIQUE && uniqueness != QUEUE_DUPLICATES) ? -EINVAL : 0;
}

int validate_uniqueness_and_whence(queue_uniqueness_t uniqueness, queue_relloc_t whence) {
    int err = validate_uniqueness(uniqueness);
    if (err != 0) {
        return err;
    }

    return validate_whence(whence);
}

inline bool embedded_queue_empty(queue_t *queue) {
    queue_assert_locked(queue);
    return queue_length(queue) ? false : true;
}

void embedded_queue_flush(queue_t *queue) {
    queue_node_t *next;

    queue_assert_locked(queue);

    forlinked(node, queue->head, next) {
        next = node->next;
        queue_node_t *prev = node->prev;

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

    if (whence != QUEUE_HEAD && whence != QUEUE_TAIL)
        return -EINVAL;

    queue_assert_locked(queue);

    if (queue_length(queue) == 0)
        return -ENOENT;

    if (whence == QUEUE_TAIL)
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

int embedded_enqueue_after(queue_t *queue, queue_node_t *node, queue_node_t *prev, queue_uniqueness_t uniqueness) {
    if (queue == NULL || node == NULL || prev == NULL)
        return -EINVAL;

    queue_assert_locked(queue);

    if (uniqueness == QUEUE_UNIQUE) {
        if (embedded_queue_contains(queue, node) == 0)
            return -EEXIST;
    }

    node->prev = prev;
    node->next = prev->next;
    prev->next = node;

    if (queue->tail == prev) {
        queue->tail = node;
    }

    queue->q_count++;

    return 0;
}

int embedded_enqueue_before(queue_t *queue, queue_node_t *node, queue_node_t *next, queue_uniqueness_t uniqueness) {
    if (queue == NULL || node == NULL || next == NULL)
        return -EINVAL;

    queue_assert_locked(queue);

    if (uniqueness == QUEUE_UNIQUE) {
        if (embedded_queue_contains(queue, node) == 0)
            return -EEXIST;
    }
    
    node->next = next;
    node->prev = next->prev;
    next->prev = node;

    if (queue->head == next) {
        queue->head = node;
    }

    queue->q_count++;

    return 0;
}

/**
 * @brief Inserts a node into an embedded queue.
 * 
 * @param queue[in] embedded queue into which to insert the node.
 * @param qnode[in] node to insert
 * @return int      returns 0, No error is indicated at the moment because this function expects the callers to validate the input. */
static inline int embedded_queue_insert(queue_t *queue, queue_node_t *qnode) {
    qnode->prev = qnode->next = NULL;

    if (queue->head == NULL) {
        queue->tail = queue->head = qnode;
    } else {
        queue->tail->next = qnode;
        qnode->prev = queue->tail;
    }

    queue->tail = qnode;
    queue->q_count++;
    return 0;
}

int embedded_enqueue(queue_t *queue, queue_node_t *qnode, queue_uniqueness_t uniqueness) {
    if (!queue || !qnode) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    if (uniqueness == QUEUE_UNIQUE){
        if (embedded_queue_contains(queue, qnode) == 0) {
            return -EEXIST;
        }
    }

    return embedded_queue_insert(queue, qnode);
}

int embedded_enqueue_head(queue_t *queue, queue_node_t *qnode, queue_uniqueness_t uniqueness) {
    if (!queue || !qnode) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    if (uniqueness == QUEUE_UNIQUE) {
        if (embedded_queue_contains(queue, qnode) == 0) {
            return -EEXIST;
        }
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
    if (whence == QUEUE_HEAD)
        return embedded_enqueue_head(queue, qnode, uniqueness);
    else if (whence == QUEUE_TAIL)
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
    if (whence == QUEUE_HEAD)
        return embedded_dequeue(queue, pnp);
    else if (whence == QUEUE_TAIL)
        return embedded_dequeue_tail(queue, pnp);
    return -EINVAL;
}

int embedded_queue_detach(queue_t *queue, queue_node_t *qnode) {
    if (!queue || !qnode) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    queue_node_t *next = qnode->next;
    queue_node_t *prev = qnode->prev;

    if (prev) {
        prev->next = next;
    }

    if (next) {
        next->prev = prev;
    }

    if (qnode == queue->head) {
        queue->head = next;
    }

    if (qnode == queue->tail) {
        queue->tail = prev;
    }

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

    if (whence != QUEUE_HEAD && whence != QUEUE_TAIL)
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

    // Remove the nodes dst source queue.
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
    if (whence == QUEUE_HEAD) {
        last->next = dst->head;
        
        if (dst->head)
            dst->head->prev = last;
        
        dst->head = first;
        
        if (dst->tail == NULL)
            dst->tail = last;
    } else if (whence == QUEUE_TAIL) {
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

static inline int queue_cmp_nodes(queue_node_t *node0, queue_node_t *node1, queue_order_t order, int (*compare)()) {
    int retval;

    if (compare) {
        if (order == QUEUE_ASCENDING) {
            retval = compare(node0, node1);
        } else if (order == QUEUE_DESCENDING) {
            retval = compare(node1, node0);
        } else { // prefer ascending order.
            retval = compare(node0, node1);
        }
    } else {
        if (!node0 || !node1)
            return -EINVAL;

        if (node0 == node1)
            retval = QUEUE_EQUAL;
        else if (node0 < node1)
            retval = QUEUE_LESSER;
        retval = QUEUE_GREATER;
    }

    return retval;
}

int enqueue_sorted(queue_t *queue, queue_node_t *qnode, queue_uniqueness_t uniqueness, queue_order_t order, int (*compare)()) {
    if (!queue || !qnode) {
        return -EINVAL;
    }

    queue_assert_locked(queue);

    qnode->next = qnode->prev = NULL;

    if (!queue->head) {
        queue->head = queue->tail = qnode;
        queue->q_count++;
        return 0;
    }

    if (uniqueness == QUEUE_UNIQUE) {
        if (embedded_queue_contains(queue, qnode) == 0)
            return -EEXIST;
    }

    qnode->next = qnode->prev = NULL;

    queue_node_t *next;
    forlinked(node, queue->head, next) {
        next = node->next;
        
        int retval = queue_cmp_nodes(qnode, node, order, compare);

        if (retval == QUEUE_EQUAL) {
            qnode->prev = node;
            qnode->next = node->next;
            node->next = qnode;
            if (qnode->next) {
                qnode->next->prev = qnode;
            } else {
                queue->tail = qnode;
            }

            break;
        } else if (retval == QUEUE_LESSER) {
            qnode->next = node;
            qnode->prev = node->prev;
            node->prev = qnode;
            if (qnode->prev) {
                qnode->prev->next = qnode;
            } else {
                queue->head = qnode;
            }
            break;
        } else if (retval == QUEUE_GREATER) {
            if (next == NULL) {
                qnode->next = NULL;
                queue->tail = qnode;

                qnode->prev = node;
                node->next = qnode;

                break;
            }
        } else {
            return retval;
        }
    }

    queue->q_count++;
    return 0;
}

static inline int validate_dest_and_srcq(queue_t *dest, queue_t *src) {
    return (!dest || !src) ? -EINVAL : 0;
}

static inline int embedded_queue_check_relloc_input(queue_t *dst, queue_t *src, queue_node_t *node, queue_uniqueness_t uniqueness, queue_relloc_t whence) {
    int err = validate_uniqueness_and_whence(uniqueness, whence);
    if (err != 0) {
        return err;
    }
    
    err = validate_dest_and_srcq(dst, src);
    if (err != 0) {
        return err;
    }
    
    if (!node) {
        return -EINVAL;
    }

    return 0;
}

int embedded_queue_relloc(queue_t *dst, queue_t *src, queue_node_t *node, queue_uniqueness_t uniqueness, queue_relloc_t whence) {
    int err = embedded_queue_check_relloc_input(dst, src, node, uniqueness, whence);
    if (err != 0) {
        return err;
    }

    queue_assert_locked(src);
    queue_assert_locked(dst);

    err = embedded_queue_remove(src, node);
    if (err != 0) {
        return err;
    }

    err = embedded_enqueue_whence(dst, node, uniqueness, whence);
    if (err != 0) {
        embedded_queue_insert(src, node);
        return err;
    }

    return 0;
}