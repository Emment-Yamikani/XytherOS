#pragma once

#include <core/types.h>
#include <core/defs.h>

/**
 * @brief Defines whether a queue permits duplicate nodes.
 *
 * QUEUE_DUPLICATES allows multiple identical qnode_t instances to be inserted.
 * QUEUE_UNIQUE rejects insertions if the node already exists in the list.
 */
typedef enum {
    QUEUE_DUPLICATES = 0,
    QUEUE_UNIQUE     = 1,
} queue_uniqueness_t;

/**
 * @brief Specifies the insertion position for relocation or migration.
 */
typedef enum {
    QUEUE_TAIL,  /**< Insert at the tail (before the sentinel or tail node). */
    QUEUE_HEAD   /**< Insert at the head (after the sentinel or head node).   */
} queue_relloc_t;

/**
 * @brief Intrusive circular doubly-linked queue node.
 *
 * This structure must be embedded inside user-defined container types.
 * It forms a node in a circular linked list where `next` and `prev`
 * always point to valid nodes — no NULL pointers appear inside the list.
 */
typedef struct qnode {
    struct qnode *prev;  /**< Previous node in the circular chain */
    void         *data;  /**< Optional user-supplied payload or container pointer */
    struct qnode *next;  /**< Next node in the circular chain */
} qnode_t;

/**
 * @brief Static initializer for a standalone qnode_t acting as a sentinel.
 */
#define QNODE_INIT(head, data) ((qnode_t){ &(head), (data), &(head) })

/* -------------------------------------------------------------------------- */
/*  Container access helpers                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Retrieves a container pointer from an embedded qnode_t.
 *
 * @param node   Pointer to qnode_t inside the container.
 * @param type   Container type.
 * @param member Name of the qnode_t field inside the container.
 */
#define qnode_container(node, type, member) \
    (type *)((node) ? container_of((node), type, member) : NULL)

/** Get the container of the next element. */
#define next_qnode_container(item, member) \
    qnode_container((item) ? (item)->member.next : NULL, typeof(*(item)), member)

/** Get the container of the previous element. */
#define prev_qnode_container(item, member) \
    qnode_container((item) ? (item)->member.prev : NULL, typeof(*(item)), member)

/* -------------------------------------------------------------------------- */
/*  Node-level iterators                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Iterate forward through all nodes in a circular queue.
 *
 * @param head Sentinel or designated start node.
 * @param node Iteration variable of type qnode_t*.
 */
#define qnode_foreach_node(head, node)        \
    for (node = (head) ? (head)->next : NULL; \
         (node) != (head); (node) = (node) ? (node)->next : NULL)

/**
 * @brief Iterate backward through all nodes in a circular queue.
 *
 * @param tail Node used as the sentinel for reverse iteration.
 * @param node Iteration variable of type qnode_t*.
 */
#define qnode_foreach_node_reverse(tail, node) \
    for (node = (tail) ? (tail)->prev : NULL;  \
         (node) != (tail); (node) = (node) ? (node)->prev : NULL)

/**
 * @brief Safe forward iteration allowing node removal during traversal.
 */
#define qnode_foreach_node_safe(head, node, n)                                    \
    for (node = (head) ? (head)->next : NULL, (n) = (node) ? (node)->next : NULL; \
         (node) != (head); (node) = (n), (n) = (node) ? (node)->next : NULL)

/**
 * @brief Safe reverse iteration allowing node removal during traversal.
 */
#define qnode_foreach_node_reverse_safe(tail, node, p)                            \
    for (node = (tail) ? (tail)->prev : NULL, (p) = (node) ? (node)->prev : NULL; \
         (node) != (tail); (node) = (p), (p) = (node) ? (node)->prev : NULL)

/* -------------------------------------------------------------------------- */
/*  Container-level iterators                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Iterate forward over entries (containers) in a circular list.
 */
#define qnode_foreach_entry(head, item, member)                           \
    for ((item) = qnode_container((head)->next, typeof(*(item)), member); \
         &((item)->member) != (head);                                     \
         (item) = next_qnode_container((item), member))

/**
 * @brief Iterate backward over entries (containers) in a circular list.
 */
#define qnode_foreach_entry_reverse(tail, item, member)                   \
    for ((item) = qnode_container((tail)->prev, typeof(*(item)), member); \
         &((item)->member) != (tail);                                     \
         (item) = prev_qnode_container((item), member))

/**
 * @brief Safe forward iteration over container entries.
 *
 * Allows removal of the current element during traversal.
 */
#define qnode_foreach_entry_safe(head, item, n, member)                   \
    for ((item) = qnode_container((head)->next, typeof(*(item)), member), \
        (n) = next_qnode_container((item), member);                       \
         &((item)->member) != (head);                                     \
         (item) = (n), (n) = next_qnode_container((n), member))

/**
 * @brief Safe reverse iteration over container entries.
 */
#define qnode_foreach_entry_reverse_safe(tail, item, p, member)           \
    for ((item) = qnode_container((tail)->prev, typeof(*(item)), member), \
        (p) = prev_qnode_container((item), member);                       \
         &((item)->member) != (tail);                                     \
         (item) = (p), (p) = prev_qnode_container((p), member))

/* -------------------------------------------------------------------------- */
/*  Utility functions                                                         */
/* -------------------------------------------------------------------------- */

/** Convert a qnode into a human-readable string. */
extern void qnode_to_string(qnode_t *node, char *buf);

/** Convert qnode->data into a human-readable string. */
extern void qnode_data_as_string(qnode_t *node, char *buf);

/** Print a queue for debugging. */
extern void qnode_print(qnode_t *head);

/** Initialize a qnode with a data pointer. */
extern int qnode_init(qnode_t *node, void *data);

/** Allocate and initialize a new qnode. */
extern int qnode_create(void *data, qnode_t **pqnode);

/** Free a dynamically allocated qnode. */
extern void qnode_destroy(qnode_t *node);

/* -------------------------------------------------------------------------- */
/*  Core queue operations                                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Check whether a node is present in the queue.
 *
 * @return 0 if present, -ENOENT if not found.
 */
extern int qnode_contains(qnode_t *head, qnode_t *node);

/** Insert a node after the tail node (enqueue at end). */
extern int qnode_enqueue(qnode_t *tail, qnode_t *node, queue_uniqueness_t uniqueness);

/** Insert a node before the head node (enqueue at front). */
extern int qnode_enqueue_head(qnode_t *head, qnode_t *node, queue_uniqueness_t uniqueness);

/** Detach a node from the list. */
extern int qnode_detach(qnode_t *node);

/** Remove a node from the list. */
extern int qnode_list_remove(qnode_t *head, qnode_t *node);
extern int qnode_remove(qnode_t *node);

/** Replace a node with another node in-place. */
extern int qnode_replace(qnode_t *qnode0, qnode_t *qnode1);

/** Insert a node immediately after the given node. */
extern int qnode_enqueue_after(qnode_t *prev, qnode_t *node, queue_uniqueness_t uniqueness);

/** Insert a node immediately before the given node. */
extern int qnode_enqueue_before(qnode_t *next, qnode_t *node, queue_uniqueness_t uniqueness);

/**
 * @brief Enqueue a node either at head or tail depending on `whence`.
 */
extern int qnode_enqueue_whence(qnode_t *head, qnode_t *qnode,
                                  queue_uniqueness_t uniq, queue_relloc_t whence);

extern int qnode_enqueue_between(qnode_t *prev, qnode_t *node, qnode_t *next);

/**
 * @brief Migrate a contiguous block of nodes from one list to another.
 *
 * @param dst    Destination list's anchor node.
 * @param src    Source list's anchor node.
 * @param pos    Zero-based starting index within `src`.
 * @param n      Number of nodes to migrate.
 * @param whence Insert at head or tail of dst.
 */
extern int qnode_migrate(qnode_t *dst, qnode_t *src,
                           usize pos, usize n, queue_relloc_t whence);

extern int qnode_relloc(qnode_t *head, qnode_t *node, queue_uniqueness_t uniq, queue_relloc_t whence);

/* -------------------------------------------------------------------------- */
/*  Sorted insertion                                                          */
/* -------------------------------------------------------------------------- */

/** Comparison results for qnode_cmp_fn_t. */
enum {
    QUEUE_EQUAL,
    QUEUE_LESSER,
    QUEUE_GREATER,
};

/** Function type for node comparison. */
typedef int (*qnode_cmp_fn_t)(qnode_t *, qnode_t *);

/** Sort order for sorted enqueue operations. */
typedef enum {
    QUEUE_ASCENDING,
    QUEUE_DESCENDING,
} queue_order_t;

/**
 * @brief Insert a node into a sorted circular queue.
 *
 * For uniqueness enforcement, QUEUE_EQUAL results are rejected if
 * uniqueness == QUEUE_UNIQUE.
 */
extern int qnode_sorted_enqueue(qnode_t *head, qnode_t *node,
                                  queue_uniqueness_t uniqueness,
                                  queue_order_t order,
                                  qnode_cmp_fn_t cmp);

/* -------------------------------------------------------------------------- */
/*  Iteration callback APIs                                                   */
/* -------------------------------------------------------------------------- */

typedef int (*qnode_into_iter_fn_t)(qnode_t *);

/**
 * @brief Apply a callback to each node in the queue.
 *
 * Iteration stops early if the callback returns non-zero.
 */
extern int qnode_into_iter(qnode_t *head, qnode_into_iter_fn_t into_iter);

extern int qnode_iter_next(qnode_t *head, qnode_t **pnext);
extern int qnode_iter_prev(qnode_t *head, qnode_t **pprev);

extern int qnode_drain(qnode_t *head);

extern bool qnode_empty_list(qnode_t *head);

extern int qnode_splice(qnode_t *list0, qnode_t *list1);

/**
 * @brief Get the length of a circularly linked list.
 *        This function does not count head.
 * @param head[in] sentinel of the list. 
 * @returns size_t
 */
extern size_t qnode_length(qnode_t *head);

/**
 * @brief Rotate the list to the left.
 * 
 * @param head[in] sentinel of the list
 * @returns int 0 for success and -errno on error.
 */
extern int qnode_rotate_left(qnode_t *head);

/**
 * @brief Rotate the list to the right.
 *
 * @param head[in] sentinel of the list
 * @returns int 0 for success and -errno on error.
 */
extern int qnode_rotate_right(qnode_t *head);

