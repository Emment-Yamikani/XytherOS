#pragma once

#include <core/types.h>
#include <sync/spinlock.h>

struct queue;

typedef enum {
    QUEUE_ALLOW_DUPLICATES  = 0,
    QUEUE_ENFORCE_UNIQUE    = 1,
} queue_uniqueness_t;

// rellocation points, used with queue rellocation functions.
typedef enum {
    QUEUE_RELLOC_TAIL,  // rellocate to the tail-end.
    QUEUE_RELLOC_HEAD   // rellocate to the front-end.
} queue_relloc_t;

typedef struct queue_node {
    struct queue_node   *prev;
    void                *data;
    struct queue_node   *next;
} queue_node_t;

typedef struct queue {
    queue_node_t    *head;
    queue_node_t    *tail;
    size_t          q_count;
    spinlock_t      q_lock;
} queue_t;

// initialize a queue struct to 'all-zeros'.
#define QUEUE_INIT()    ((queue_t){0})

// get a preallocated queue pointer at compile time.
#define QUEUE_NEW()     (&QUEUE_INIT())

/**
 * @brief shorthand for declaring a queue
 * this uses QUEUE_NEW() to provided the initialize struct
 * returning a pointer to it.
 * @param name is the name of the queue pointer.
 */
#define QUEUE(name)     queue_t *name = QUEUE_NEW()

#define queue_assert(queue)         ({ assert((queue), "No queue.\n"); })
#define queue_lock(queue)           ({ queue_assert(queue); spin_lock(&(queue)->q_lock); })
#define queue_unlock(queue)         ({ queue_assert(queue); spin_unlock(&(queue)->q_lock); })
#define queue_trylock(queue)        ({ queue_assert(queue); spin_trylock(&(queue)->q_lock); })
#define queue_islocked(queue)       ({ queue_assert(queue); spin_islocked(&(queue)->q_lock); })
#define queue_recursive_lock(queue) ({ queue_assert(queue); spin_recursive_lock(&(queue)->q_lock); })
#define queue_assert_locked(queue)  ({ queue_assert(queue); spin_assert_locked(&(queue)->q_lock); })

// initialize a queue at runtime with this macro.
#define INIT_QUEUE(queue) ({           \
    queue_assert(queue);               \
    memset(queue, 0, sizeof *(queue)); \
    (queue)->q_lock = SPINLOCK_INIT(); \
})

/**
 * @brief Iterates over a queue's node.
 * caller must have held queue->q_lock prior to calling this macro.
 */
#define queue_foreach(queue, type, item)                                            \
    queue_assert_locked(queue);                                                     \
    for (queue_node_t *item##_node = (queue) ? (queue)->head : NULL,                \
                      *next_##item##_node = item##_node ? item##_node->next : NULL; \
                      item##_node != NULL; item##_node = next_##item##_node,        \
                      next_##item##_node = item##_node ? item##_node->next : NULL)  \
        for (type item = (type)item##_node->data; item##_node != NULL; item##_node = NULL)

#define queue_foreach_reverse(queue, type, item)                                   \
    queue_assert_locked(queue);                                                    \
    for (queue_node_t *item##_node = (queue) ? (queue)->tail : NULL,               \
                      *prev##item##_node = item##_node ? item##_node->prev : NULL; \
                      item##_node != NULL; item##_node = prev##item##_node,        \
                      prev##item##_node = item##_node ? item##_node->prev : NULL)  \
        for (type item = (type)item##_node->data; item##_node != NULL; item##_node = NULL)

extern int queue_init(queue_t *queue);

extern int queue_node_init(queue_node_t *qnode, void *data);

// @brief free memory allocated via queue_alloc()
extern void queue_free(queue_t *queue);

// flushes all data currently on the queue specified by parameter 'queue'
extern void queue_flush(queue_t *queue);

// allocates a queue queue returning a pointer to the newly allocated queue.
extern int queue_alloc(queue_t **pqp);

// returns the number of items currently on the queue specified by param 'queue'.
extern size_t queue_count(queue_t *queue);

/**
 * @brief Used to take a peek at the front
 * or back-end of the queue specified by queue.
 *
 * @param[in] queue the queue to be peeked
 * @param[in] whence If 'QUEUE_RELLOC_TAIL', retrieves the tail node; otherwise, retrieves the head.
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
extern int queue_contains(queue_t *queue, void *data, queue_node_t **pnp);

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
extern int enqueue(queue_t *queue, void *data, queue_uniqueness_t uniqueness, queue_node_t **pnp);

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
extern int enqueue_head(queue_t *queue, void *data, queue_uniqueness_t uniqueness, queue_node_t **pnp);

extern int enqueue_whence(queue_t *queue, void *data, queue_uniqueness_t uniqueness, queue_relloc_t whence, queue_node_t **pnp);

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
extern int queue_remove_node(queue_t *queue, queue_node_t *__node);

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
extern int queue_rellocate_node(queue_t *queue, queue_node_t *node, queue_relloc_t whence);

// same as above only difference is this take a data pointer not a node.
extern int queue_rellocate(queue_t *queue, void *data, queue_relloc_t whence);

extern int queue_node_migrate(queue_t *dstq, queue_t *srcq, usize start_pos, usize num_nodes, queue_relloc_t whence);

extern int queue_move(queue_t *dstq, queue_t *srcq, queue_relloc_t whence);

extern int queue_replace(queue_t *queue, void *data0, void *data1);

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
 * @param[in] whence If 'QUEUE_RELLOC_TAIL', retrieves the tail node; otherwise, retrieves the head.
 * @param[out] pnp Pointer to pointer to the requested node, or NULL if the queue is empty.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_queue_peek(queue_t *queue, queue_relloc_t whence, queue_node_t **pnp);

/**
 * @brief Checks if a node exists in the queue.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] node Pointer to the node to check.
 * @return 0 if the node is found, -error otherwise.
 */
extern int embedded_queue_contains(queue_t *queue, queue_node_t *qnode);

/**
 * @brief Removes and returns the head node from the queue.
 *
 * The caller is responsible for managing the memory of the dequeued node.
 *
 * @param[in] queue Pointer to the queue.
 * @param[out] pnp Pointer to pointer to the dequeued node, or NULL if the queue is empty.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_dequeue(queue_t *queue, queue_node_t **pnp);

/**
 * @brief Removes and returns the tail node from the queue.
 *
 * @param[in] queue Pointer to the queue.
 * @param[out] pnp Pointer to pointer to the dequeued node, or NULL if the queue is empty.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_dequeue_tail(queue_t *queue, queue_node_t **pnp);

/**
 * @brief Removes and returns a node from a specified position.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] whence Position from which to remove the node.
 * @param[out] pnp Pointer to pointer to the dequeued node, or NULL if not found.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_dequeue_whence(queue_t *queue, queue_relloc_t whence, queue_node_t **pnp);

/**
 * @brief Enqueues a node at the tail of the queue.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] node Pointer to the node to enqueue.
 * @param[in] uniqueness Defines whether duplicates are allowed.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_enqueue(queue_t *queue, queue_node_t *qnode, queue_uniqueness_t uniqueness);

/**
 * @brief Enqueues a node at the head of the queue.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] node Pointer to the node to enqueue.
 * @param[in] uniqueness Defines whether duplicates are allowed.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_enqueue_head(queue_t *queue, queue_node_t *qnode, queue_uniqueness_t uniqueness);

/**
 * @brief Enqueues a node at a specific position in the queue.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] node Pointer to the node to enqueue.
 * @param[in] uniqueness Defines whether duplicates are allowed.
 * @param[in] whence Position where the node should be inserted.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_enqueue_whence(queue_t *queue, queue_node_t *qnode, queue_uniqueness_t uniqueness, queue_relloc_t whence);

/**
 * @brief Removes a specific node from the queue.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] node Pointer to the node to remove.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_queue_remove(queue_t *queue, queue_node_t *qnode);

/**
 * @brief Like embedded_queue_remove(); except it doesn't check the node's existence.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] node Pointer to the node to remove.
 * @return 0 on success, non-zero on failure.
 */
int embedded_queue_detach(queue_t *queue, queue_node_t *qnode);

/**
 * @brief Replaces a node in the queue with another.
 *
 * @param[in] queue Pointer to the queue.
 * @param[in] node0 Pointer to the node to be replaced.
 * @param[in] node1 Pointer to the replacement node.
 * @return 0 on success, non-zero on failure.
 */
extern int embedded_queue_replace(queue_t *queue, queue_node_t *qnode0, queue_node_t *qnode1);

extern int embedded_queue_migrate(queue_t *dst, queue_t *src, usize pos, usize n, queue_relloc_t whence);

#define embedded_queue_foreach(queue, type, item, member)                                                        \
    queue_assert_locked(queue);                                                                                  \
    for (queue_node_t *item##_node = (queue)->head, *next_item##_node = item##_node ? item##_node->next : NULL;  \
         item##_node; item##_node = next_item##_node, next_item##_node = item##_node ? item##_node->next : NULL) \
        for (type *item = container_of(item##_node, type, member); item##_node; item##_node = NULL)

#define embedded_queue_foreach_reverse(queue, type, item, member)                                                \
    queue_assert_locked(queue);                                                                                  \
    for (queue_node_t *item##_node = (queue)->tail, *prev_item##_node = item##_node ? item##_node->prev : NULL;  \
         item##_node; item##_node = prev_item##_node, prev_item##_node = item##_node ? item##_node->prev : NULL) \
        for (type *item = container_of(item##_node, type, member); item##_node; item##_node = NULL)
