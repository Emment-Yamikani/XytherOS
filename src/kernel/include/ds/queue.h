#pragma once

#include <sync/spinlock.h>
#include <ds/internal_queue.h>

typedef struct queue {
    size_t      q_count;
    qnode_t     q_sentinel;
    spinlock_t  q_lock;
} queue_t;

#define queue_assert(queue)         ({ assert((queue), "No queue.\n"); })
#define queue_lock(queue)           ({ queue_assert(queue); spin_lock(&(queue)->q_lock); })
#define queue_unlock(queue)         ({ queue_assert(queue); spin_unlock(&(queue)->q_lock); })
#define queue_trylock(queue)        ({ queue_assert(queue); spin_trylock(&(queue)->q_lock); })
#define queue_islocked(queue)       ({ queue_assert(queue); spin_islocked(&(queue)->q_lock); })
#define queue_recursive_lock(queue) ({ queue_assert(queue); spin_recursive_lock(&(queue)->q_lock); })
#define queue_assert_locked(queue)  ({ queue_assert(queue); spin_assert_locked(&(queue)->q_lock); })

// Macro to create the initialization constant
// 'q' is a placeholder for the queue variable name
#define QUEUE_INLINE_INIT(q) (queue_t)       \
{                                            \
    .q_sentinel = {                          \
        .data = NULL,                        \
        .next = &(q).q_sentinel,             \
        .prev = &(q).q_sentinel,             \
    },                                       \
    .q_count = 0, .q_lock = SPINLOCK_INIT(), \
}

/**
 * @brief Declare a static queue.
 *
 * This macro places the queue descriptor.
 **/
#define QUEUE(ownership, queue_name) ownership queue_t queue_name = QUEUE_INLINE_INIT(queue_name)

#define queue_foreach_node(queue, node) \
    queue_assert_locked(queue);         \
    qnode_foreach_node(&(queue)->q_sentinel, node)

#define queue_foreach_node_reverse(queue, node) \
    queue_assert_locked(queue);                 \
    qnode_foreach_node_reverse(&(queue)->q_sentinel, node)

#define queue_foreach_node_safe(queue, node, next) \
    queue_assert_locked(queue);                 \
    qnode_foreach_node_safe(&(queue)->q_sentinel, node, next)

#define queue_foreach_node_reverse_safe(queue, node, prev) \
    queue_assert_locked(queue);                         \
    qnode_foreach_node_reverse_safe(&(queue)->q_sentinel, node, prev)

#define queue_foreach_entry(queue, item, member) \
    queue_assert_locked(queue);                  \
    qnode_foreach_entry((&(queue)->q_sentinel), (item), member)

#define queue_foreach_entry_reverse(queue, item, member) \
    queue_assert_locked(queue);                          \
    qnode_foreach_entry_reverse((&(queue)->q_sentinel), (item), member)

#define queue_foreach_entry_safe(queue, item, n, member) \
    queue_assert_locked(queue);                          \
    qnode_foreach_entry_safe((&(queue)->q_sentinel), (item), (n), member)

#define queue_foreach_entry_reverse_safe(queue, item, p, member) \
    queue_assert_locked(queue);                                  \
    qnode_foreach_entry_reverse_safe((&(queue)->q_sentinel), (item), (p), member)

extern int embedded_queue_init(queue_t *queue);

extern void embedded_queue_print(queue_t *queue);

extern void embedded_queue_display(queue_t *queue);

extern size_t embedded_queue_length(queue_t *queue);

extern int embedded_queue_create(queue_t **pqueue);

extern int embedded_queue_drain(queue_t *queue, qnode_into_iter_fn_t qnode_at_drain);

extern void embedded_queue_destroy(queue_t *queue, qnode_into_iter_fn_t node_at_destroy);

typedef int (*embedded_queue_into_iter_fn_t)(queue_t *queue, qnode_t *node, qnode_into_iter_fn_t into_iter);
extern int embedded_queue_into_iter(queue_t *queue, embedded_queue_into_iter_fn_t into_iter, qnode_into_iter_fn_t qnode_into_iter);

/**
 * @brief Removes all nodes from the queue without freeing them.
 *
 * This function clears all nodes from the queue but does not deallocate them,
 * making it suitable for embedded nodes.
 *
 * @param[in] queue Pointer to the queue.
 */
extern void embedded_queue_flush(queue_t *queue);

/**
 * @brief Retrieves the head or tail node without removing it.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] whence If 'QUEUE_TAIL', retrieves the tail node; otherwise, retrieves the head.
 * @param[out] pnp Pointer to pointer to the requested node, or NULL if the queue is empty.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_queue_peek(queue_t *queue, queue_relloc_t whence, qnode_t **pnp);

/**
 * @brief Checks if a node exists in the queue.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] node Pointer to the node to check.
 * @return 0 if the node is found, -error otherwise.
 */
extern int embedded_queue_contains(queue_t *queue, qnode_t *qnode);

/**
 * @brief Removes and returns the head node from the queue.
 *
 * The caller is responsible for managing the memory of the dequeued node.
 *
 * @param[in] queue Pointer to the queue.
 * @param[out] pnp Pointer to pointer to the dequeued node, or NULL if the queue is empty.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_dequeue(queue_t *queue, qnode_t **pnp);

/**
 * @brief Removes and returns the tail node from the queue.
 *
 * @param[in] queue Pointer to the queue.
 * @param[out] pnp Pointer to pointer to the dequeued node, or NULL if the queue is empty.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_dequeue_tail(queue_t *queue, qnode_t **pnp);

/**
 * @brief Removes and returns a node from a specified position.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] whence Position from which to remove the node.
 * @param[out] pnp Pointer to pointer to the dequeued node, or NULL if not found.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_dequeue_whence(queue_t *queue, queue_relloc_t whence, qnode_t **pnp);

/**
 * @brief Enqueues a node at the tail of the queue.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] node Pointer to the node to enqueue.
 * @param[in] uniqueness Defines whether duplicates are allowed.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_enqueue(queue_t *queue, qnode_t *qnode, queue_uniqueness_t uniqueness);

/**
 * @brief Enqueues a node at the head of the queue.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] node Pointer to the node to enqueue.
 * @param[in] uniqueness Defines whether duplicates are allowed.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_enqueue_head(queue_t *queue, qnode_t *qnode, queue_uniqueness_t uniqueness);

/**
 * @brief Enqueues a node at a specific position in the queue.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] node Pointer to the node to enqueue.
 * @param[in] uniqueness Defines whether duplicates are allowed.
 * @param[in] whence Position where the node should be inserted.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_enqueue_whence(queue_t *queue, qnode_t *qnode, queue_uniqueness_t uniqueness, queue_relloc_t whence);

extern int enqueue_sorted(queue_t *queue, void *data, queue_uniqueness_t uniqueness, queue_order_t order, qnode_cmp_fn_t compare, qnode_t **pnp);

/**
 * @brief Removes a specific node from the queue.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] node Pointer to the node to remove.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_queue_remove(queue_t *queue, qnode_t *qnode);

/**
 * @brief Replaces a node in the queue with another.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] node0 Pointer to the node to be replaced.
 * @param[in] node1 Pointer to the replacement node.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_queue_replace(queue_t *queue, qnode_t *qnode0, qnode_t *qnode1);

extern int embedded_enqueue_after(queue_t *queue, qnode_t *prev, qnode_t *node, queue_uniqueness_t uniqueness);
extern int embedded_enqueue_before(queue_t *queue, qnode_t *next, qnode_t *qnode, queue_uniqueness_t uniqueness);

extern int embedded_queue_migrate(queue_t *dst, queue_t *src, usize pos, usize n, queue_relloc_t whence);

extern bool embedded_queue_empty(queue_t *queue);

extern int embedded_sorted_enqueue(queue_t *queue, qnode_t *qnode, queue_uniqueness_t uniqueness, queue_order_t order, qnode_cmp_fn_t compare);

extern int embedded_qnode_rotate_left(queue_t *queue);

extern int embedded_qnode_rotate_right(queue_t *queue);

/**
 * @brief 
 * 
 * @param dst[in] 
 * @param src[in] 
 * @param node[in] 
 * @param uniqueness[in] 
 * @param whence[in] 
 * @return int 
 */
extern int embedded_queue_relloc(queue_t *dst, queue_t *src, qnode_t *node, queue_uniqueness_t uniqueness, queue_relloc_t whence);

extern void queue_display(queue_t *queue);

extern int queue_init(queue_t *queue);

extern int queue_qnode_init(qnode_t *qnode, void *data);

// @brief free memory allocated via queue_create()
extern void queue_destroy(queue_t *queue);

// flushes all data currently on the queue specified by parameter 'queue'
extern void queue_flush(queue_t *queue);

// allocates a queue queue returning a pointer to the newly allocated queue.
extern int queue_create(queue_t **pqp);

// returns the number of items currently on the queue specified by param 'queue'.
extern size_t queue_length(queue_t *queue);

extern bool queue_empty(queue_t *queue);

/**
 * @brief Used to take a peek at the front
 * or back-end of the queue specified by queue.
 *
 * @param[in] queue the queue to be peeked
 * @param[in] whence If 'QUEUE_TAIL', retrieves the tail node; otherwise, retrieves the head.
 *  otherwise the front-end data is peeked.
 * @param[out] pdp pointer to a location to store a pointer to the peeked data.
 * @return int 0 on success, otherwise reports the reports the error that has occured.
 */
extern int queue_peek(queue_t *queue, queue_relloc_t whence, void **pdp);

/**
 * @brief checks the availability of the data specified by data.
 *
 * @param[in] queue queue being queried for the data.
 * @param[in] data the data to be queried.
 * @param[out] pnp pointer in which a pointer to node containing
 * the data.
 * @return int 0 on success otherwise an error code is returned.
 */
extern int queue_contains(queue_t *queue, void *data, qnode_t **pnp);

/**
 * @brief Dequeue a data item from the queue.
 *  data once is removed from the queue once dequeued
 * @param[in] queue queue from which data is retrieved.
 * @param[out] pdp pointer in which to return the data.
 * @return int 0 on success, otherwise error code is returned.
 */
extern int dequeue(queue_t *queue, void **pdp);

/**
 * @brief same as dequeue(), except the retrieval happens at the tail-end.
 *
 * @param[in] queue queue from which data is to be retrieved.
 * @param[out] pdp pointer in which to return the data.
 * @return int 0 on success, otherwise error code is returned.
 */
extern int dequeue_tail(queue_t *queue, void **pdp);

extern int dequeue_whence(queue_t *queue, queue_relloc_t whence, void **pdp);

/**
 * @brief enqueue a data item onto the queue.
 *
 * @param[in] queue queue on which the datum is enqueued.
 * @param[in] data data to be enqueued.
 * @param[in] uniqueness if non-zero, enqueue() will deny multiple data items
 * that is similar.
 * @param[out] pnp if non-null, returned pointer to the node holding this data.
 * @return int 0 on success or error code otherwise.
 */
extern int enqueue(queue_t *queue, void *data, queue_uniqueness_t uniqueness, qnode_t **pnp);

/**
 * @brief Same as enqueue(), except the data is enqueued
 * at the front-end of the queue.
 *
 * @param[in] queue queue on which the datum is enqueued.
 * @param[in] data data to be enqueued.
 * @param[in] uniqueness if non-zero, enqueue() will deny multiple data items
 * that is similar.
 * @param[out] pnp if non-null, returned pointer to the node holding this data.
 * @return int 0 on success or error code otherwise.
 */
extern int enqueue_head(queue_t *queue, void *data, queue_uniqueness_t uniqueness, qnode_t **pnp);

extern int enqueue_whence(queue_t *queue, void *data, queue_uniqueness_t uniqueness, queue_relloc_t whence, qnode_t **pnp);

/**
 * @brief Removes a data item from the queue
 *
 * @param[in] queue queue from which the data item is removed.
 * @param[in] data the data to be removed.
 * @return int 0 on success, otherwise and error code is returned.
 */
extern int queue_remove(queue_t *queue, void *data);

/**
 * @brief Removes a data node from the queue
 *
 * @param[in] queue queue from which the node item is removed.
 * @param[in] data the node to be removed.
 * @return int 0 on success, otherwise and error code is returned.
 */
extern int queue_remove_node(queue_t *queue, qnode_t *__node);

/**
 * @brief Rellocates  a data item to the fron or back depending
 * on the value of 'head'.
 *
 * @param[in] queue queue on which to apply the operation.
 * @param[in] node contains the data to be rellocated.
 * @param[in] whence specifies where the data is the be rellocated to.
 *  see above enum queue_relloc_t typedef.
 * @return int 0 on success, otherwise and error code is returned.
 */
extern int queue_rellocate_node(queue_t *queue, qnode_t *node, queue_relloc_t whence);

// same as above only difference is this take a data pointer not a node.
extern int queue_rellocate(queue_t *queue, void *data, queue_relloc_t whence);

extern int queue_qnode_migrate(queue_t *dstq, queue_t *srcq, usize start_pos, usize num_nodes, queue_relloc_t whence);

extern int queue_move(queue_t *dstq, queue_t *srcq, queue_relloc_t whence);

extern int queue_replace(queue_t *queue, void *data0, void *data1);

extern int queue_rotate_left(queue_t *queue);

extern int queue_rotate_right(queue_t *queue);