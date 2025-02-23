#include <bits/errno.h>
#include <core/debug.h>
#include <ds/queue.h>
#include <mm/kalloc.h>
#include <string.h>

int queue_node_init(queue_node_t *qnode, void *data) {
    if (qnode == NULL)
        return -EINVAL;

    qnode->data = data;
    qnode->prev = qnode->next = NULL;
    return 0;
}

int queue_init(queue_t *queue) {
    if (queue == NULL)
        return -EINVAL;

    queue->q_count = 0;
    queue->head = queue->tail = NULL;
    spinlock_init(&queue->q_lock);
    return 0;
}

int queue_alloc(queue_t **pqp) {
    queue_t *queue = NULL;

    if (pqp == NULL)
        return -EINVAL;

    if ((queue = kmalloc(sizeof *queue)) == NULL)
        return -ENOMEM;

    queue_init(queue);
    *pqp = queue;
    return 0;
}

void queue_flush(queue_t *queue) {
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
        kfree(node);
    }

    queue->tail = NULL;
    queue->head = NULL;
}

void queue_free(queue_t *queue) {
    queue_recursive_lock(queue);
    queue_flush(queue);
    queue_unlock(queue);
    kfree(queue);
}

usize queue_count(queue_t *queue) {
    queue_assert_locked(queue);
    return queue->q_count;
}

int queue_peek(queue_t *queue, queue_relloc_t whence, void **pdp) {
    queue_assert_locked(queue);

    if (queue == NULL || pdp == NULL)
        return -EINVAL;

    if (whence != QUEUE_RELLOC_HEAD && whence != QUEUE_RELLOC_TAIL)
        return -EINVAL;

    if (queue_count(queue) == 0)
        return -ENOENT;
    
    if (whence == QUEUE_RELLOC_TAIL)
        *pdp = queue->tail->data;
    else *pdp = queue->head->data;
    
    return 0;
}

int queue_contains(queue_t *queue, void *data, queue_node_t **pnp) {
    queue_node_t *next = NULL;
    queue_assert_locked(queue);
    if (queue == NULL)
        return -EINVAL;

    forlinked(node, queue->head, next) {
        next = node->next;
        if (node->data == data)
        {
            if (pnp)
                *pnp = node;
            return 0;
        }
    }

    return -ENOENT;
}

int enqueue(queue_t *queue, void *data, queue_uniqueness_t uniqueness, queue_node_t **pnp) {
    queue_node_t *node = NULL;
    queue_assert_locked(queue);

    if (queue == NULL)
        return -EINVAL;

    if (uniqueness == QUEUE_ENFORCE_UNIQUE){
        if (queue_contains(queue, data, NULL) == 0)
            return -EEXIST;
    }

    if ((node = kmalloc(sizeof(*node))) == NULL)
        return -ENOMEM;

    memset(node, 0, sizeof *node);

    node->data = data;

    if (queue->head == NULL)
        queue->head = node;
    else {
        queue->tail->next = node;
        node->prev = queue->tail;
    }

    queue->tail = node;
    queue->q_count++;

    if (pnp)
        *pnp = node;

    return 0;
}

int enqueue_head(queue_t *queue, void *data, queue_uniqueness_t uniqueness, queue_node_t **pnp) {
    queue_node_t *node = NULL;

    queue_assert_locked(queue);

    if (queue == NULL)
        return -EINVAL;

    if (uniqueness == QUEUE_ENFORCE_UNIQUE) {
        if (queue_contains(queue, data, NULL) == 0)
            return -EEXIST;
    }

    if ((node = kmalloc(sizeof(*node))) == NULL)
        return -ENOMEM;

    memset(node, 0, sizeof *node);

    node->data = data;

    if (queue->head == NULL) {
        queue->tail = node;
    } else {
        queue->head->prev = node;
        node->next = queue->head;
    }

    queue->head = node;
    queue->q_count++;

    if (pnp)
        *pnp = node;
    return 0;
}

int enqueue_whence(queue_t *queue, void *data, queue_uniqueness_t uniqueness, queue_relloc_t whence, queue_node_t **pnp) {
    queue_assert_locked(queue);
    if (whence == QUEUE_RELLOC_HEAD)
        return enqueue_head(queue, data, uniqueness, pnp);
    else if (whence == QUEUE_RELLOC_TAIL)
        return enqueue(queue, data, uniqueness, pnp);
    return -EINVAL;
}

int dequeue(queue_t *queue, void **pdp) {
    queue_node_t *node = NULL, *prev = NULL, *next = NULL;
    queue_assert_locked(queue);

    if (queue == NULL || pdp == NULL)
        return -EINVAL;

    node = queue->head;
    if (node) {
        *pdp = node->data;
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
        kfree(node);

        return 0;
    }

    return -ENOENT;
}

int dequeue_tail(queue_t *queue, void **pdp) {
    queue_node_t *next = NULL, *node = NULL, *prev = NULL;

    queue_assert_locked(queue);
    if (queue == NULL || pdp == NULL)
        return -EINVAL;

    node = queue->tail;
    
    if (node) {
        next = node->next;
        *pdp = node->data;
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
        kfree(node);

        return 0;
    }

    return -ENOENT;
}

int dequeue_whence(queue_t *queue, queue_relloc_t whence, void **pdp) {
    queue_assert_locked(queue);
    if (whence == QUEUE_RELLOC_HEAD)
        return dequeue(queue, pdp);
    else if (whence == QUEUE_RELLOC_TAIL)
        return dequeue_tail(queue, pdp);
    return -EINVAL;
}

int queue_remove_node(queue_t *queue, queue_node_t *__node) {
    queue_node_t *next = NULL, *prev = NULL;
    queue_assert_locked(queue);
    if (queue == NULL || __node == NULL)
        return -EINVAL;

    forlinked(node, queue->head, next) {
        next = node->next;
        prev = node->prev;
        if (__node == node) {
            if (prev)
                prev->next = next;
            if (next)
                next->prev = prev;
            if (node == queue->head)
                queue->head = next;
            if (node == queue->tail)
                queue->tail = prev;

            queue->q_count--;
            kfree(node);
            return 0;
        }
    }

    return -ENOENT;
}

int queue_remove(queue_t *queue, void *data) {
    queue_node_t *next = NULL, *prev = NULL;
    queue_assert_locked(queue);

    if (queue == NULL)
        return -EINVAL;

    forlinked(node, queue->head, next) {
        next = node->next;
        prev = node->prev;
        if (node->data == data) {
            if (prev)
                prev->next = next;
            if (next)
                next->prev = prev;
            if (node == queue->head)
                queue->head = next;
            if (node == queue->tail)
                queue->tail = prev;

            queue->q_count--;
            kfree(node);
            return 0;
        }
    }

    return -ENOENT;
}

int queue_rellocate_node(queue_t *queue, queue_node_t *node, queue_relloc_t whence) {
    queue_node_t *next = NULL;
    queue_node_t *prev = NULL;

    if (queue == NULL || node == NULL)
        return -EINVAL;

    queue_assert_locked(queue);
    
    if (whence != QUEUE_RELLOC_HEAD && whence != QUEUE_RELLOC_TAIL)
        return -EINVAL;
    
    prev = node->prev;
    next = node->next;

    if (next)
        next->prev = prev;
    if (prev)
        prev->next = next;
    if (queue->head == node)
        queue->head = next;
    if (queue->tail == node)
        queue->tail = prev;

    node->prev = NULL;
    node->next = NULL;

    switch (whence) {
    case QUEUE_RELLOC_HEAD:
        if (queue->head == NULL) {
            queue->tail = node;
        } else {
            queue->head->prev = node;
            node->next = queue->head;
        }
        queue->head = node;
        break;
    case QUEUE_RELLOC_TAIL:
        if (queue->head == NULL) {
            queue->head = node;
        } else {
            queue->tail->next = node;
            node->prev = queue->tail;
        }
        queue->tail = node;
        break;
    }

    return 0;
}

int queue_rellocate(queue_t *queue, void *data, queue_relloc_t whence) {
    int          err   = 0;
    queue_node_t *node = NULL;
    if (queue == NULL)
        return -EINVAL;

    queue_assert_locked(queue);
    
    // TODO: may need to keep a queue node ptr to quicken the rellocation
    // as hat will eliminate the need for iteration on the queue searching
    // for the data item.    
    if ((err = queue_contains(queue, data, &node)))
        return err;

    return queue_rellocate_node(queue, node, whence);
}

/**
 * @brief Migrates a specified number of nodes from a source queue to a destination queue.
 *
 * This function detaches a contiguous range of nodes starting from a given position
 * in the source queue and attaches them to the head or tail of the destination queue,
 * as specified by the `whence` parameter. The function assumes that locks on both
 * the source and destination queues are held prior to calling.
 *
 * @param dstq Pointer to the destination queue where the nodes will be migrated.
 * @param srcq Pointer to the source queue from which the nodes will be migrated.
 * @param start_pos The starting position (0-based index) of the nodes in the source queue
 *                  to begin migration.
 * @param num_nodes The number of nodes to migrate from the source queue.
 * @param whence Specifies where to attach the nodes in the destination queue.
 *               - `QUEUE_RELLOC_HEAD`: Attach nodes to the head of the destination queue.
 *               - `QUEUE_RELLOC_TAIL`: Attach nodes to the tail of the destination queue.
 *
 * @return int
 * - `0`: Success, the nodes were successfully migrated.
 * - `-EINVAL`: Failure, due to one of the following reasons:
 *     - `start_pos` or `num_nodes` is invalid (out of bounds or exceeds the queue size).
 *     - The source queue does not contain sufficient nodes.
 *     - Invalid value provided for `whence`.
 *
 * @note
 * - The function assumes that the caller has already acquired locks on both
 *   the source (`srcq`) and destination (`dstq`) queues before invoking it.
 * - The `queue` field in each migrated node is updated to point to the destination queue.
 *
 * @example
 * Example usage of `queue_node_migrate`:
 *
 * ```c
 * queue_t src_queue = QUEUE_INIT();
 * queue_t dst_queue = QUEUE_INIT();
 *
 * // Populate src_queue with data...
 *
 * queue_lock(&src_queue);
 * queue_lock(&dst_queue);
 *
 * size_t start_pos = 2;     // Start from the 3rd node
 * size_t num_nodes = 3;     // Migrate 3 nodes
 * queue_relloc_t whence = QUEUE_RELLOC_TAIL; // Attach to the tail of dst_queue
 *
 * int result = queue_node_migrate(&dst_queue, &src_queue, start_pos, num_nodes, whence);
 *
 * queue_unlock(&dst_queue);
 * queue_unlock(&src_queue);
 *
 * if (result == 0) {
 *     printf("Nodes migrated successfully.\n");
 * } else {
 *     printf("Failed to migrate nodes.\n");
 * }
 * ```
 */
int queue_node_migrate(queue_t *dstq, queue_t *srcq, usize start_pos, usize num_nodes, queue_relloc_t whence) {
    usize        index       = 0;
    queue_node_t *node       = NULL;
    queue_node_t *last_node  = NULL;
    queue_node_t *first_node = NULL;

    // Ensure both queues are valid and locked
    queue_assert_locked(srcq);
    queue_assert_locked(dstq);

    if ((start_pos >= srcq->q_count) || (num_nodes == 0) || ((start_pos + num_nodes) > srcq->q_count)) {
        return -EINVAL; // Error: invalid position or range
    }

    if (whence != QUEUE_RELLOC_HEAD && whence != QUEUE_RELLOC_TAIL)
        return -EINVAL; // Error: invalid rellocation position

    index   = 0;
    node    = srcq->head;

    // Traverse the source queue to the starting position
    while (node && index < start_pos) {
        node = node->next;
        index++;
    }

    if (!node) {
        return -ENOENT; // Error: starting node not found
    }

    // Extract nodes from the source queue
    first_node = node;
    last_node  = node;

    for (usize i = 1; i < num_nodes; i++) {
        last_node = last_node->next;
    }

    // Update pointers to remove the nodes from the source queue
    if (first_node->prev) {
        first_node->prev->next = last_node->next;
    } else {
        srcq->head = last_node->next; // Update head if removing from the start
    }

    if (last_node->next) {
        last_node->next->prev = first_node->prev;
    } else {
        srcq->tail = first_node->prev; // Update tail if removing from the end
    }

    srcq->q_count -= num_nodes;

    // Detach the nodes to migrate
    first_node->prev = NULL;
    last_node->next  = NULL;

    // Update the node->queue pointer to point to dstq
    forlinked(node, first_node, node->next) {
        // debug("node{%p}->{%p}\n", node, node->next);
    }

    // Insert the nodes into the destination queue
    if (whence == QUEUE_RELLOC_HEAD) {
        last_node->next = dstq->head;
        if (dstq->head) {
            dstq->head->prev = last_node;
        }
        dstq->head = first_node;
        if (!dstq->tail) {
            dstq->tail = last_node; // Update tail if the queue was empty
        }
    } else if (whence == QUEUE_RELLOC_TAIL) {
        first_node->prev = dstq->tail;
        if (dstq->tail) {
            dstq->tail->next = first_node;
        }
        dstq->tail = last_node;
        if (!dstq->head) {
            dstq->head = first_node; // Update head if the queue was empty
        }
    }

    dstq->q_count += num_nodes;

    return 0; // Success
}

int queue_move(queue_t *dstq, queue_t *srcq, queue_relloc_t whence) {
    // Ensure both queues are valid and locked
    queue_assert_locked(srcq);
    queue_assert_locked(dstq);

    if (whence != QUEUE_RELLOC_HEAD && whence != QUEUE_RELLOC_TAIL)
        return -EINVAL; // Error: invalid rellocation position
    return queue_node_migrate(dstq, srcq, 0, srcq->q_count, whence);
}

/**
 * @brief Replaces a data entry in the queue.
 *
 * This function traverses the queue to find a node containing `data0` and replaces
 * it with `data1`. The caller must ensure that the queue lock is held before calling
 * this function.
 *
 * @param queue The queue to search in.
 * @param data0 The data to search for and replace.
 * @param data1 The new data to replace `data0` with.
 *
 * @return int 0 on success, -ENOENT if `data0` is not found.
 *
 * @note The queue lock must be held before calling this function.
 * @note data0 and data1 cannot be the same.
 */
int queue_replace(queue_t *queue, void *data0, void *data1) {
    if (!queue || data0 == data1) {
        return -EINVAL; // Invalid arguments.
    }

    queue_assert_locked(queue);

    // Iterate through the queue to find the node containing `data0`.
    forlinked(node, queue->head, node->next) {
        if (node->data == data0) {
            node->data = data1;
            return 0;
        }
    }

    // If we reach here, `data0` was not found.
    return -ENOENT; // No such entry.
}
