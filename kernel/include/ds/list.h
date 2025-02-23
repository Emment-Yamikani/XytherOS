#pragma once

#include <core/defs.h>

/**
 * @file list.h
 * @brief A generic doubly linked list implementation with a sentinel node.
 *
 * This header provides macros and functions to manipulate a doubly linked list.
 * The list is circular and uses a sentinel node to simplify edge cases.
 */

/**
 * @struct list_head_t
 * @brief Represents a node in the doubly linked list.
 *
 * Each node contains pointers to the previous and next nodes in the list.
 */
typedef struct list_head_t list_head_t;
struct list_head_t {
    list_head_t *prev; /**< Pointer to the previous node in the list. */
    list_head_t *next; /**< Pointer to the next node in the list. */
};

/**
 * @def LIST_HEAD_INIT(head)
 * @brief Initializes a list head with itself as the previous and next node.
 *
 * @param head The list head to initialize.
 */
#define LIST_HEAD_INIT(head) { &(head), &(head) }

/**
 * @def LIST_HEAD(name)
 * @brief Declares and initializes a list head.
 *
 * @param name The name of the list head.
 */
#define LIST_HEAD(name) \
    list_head_t name = LIST_HEAD_INIT(name)

/**
 * @brief Initializes a list head.
 *
 * @param list The list head to initialize.
 */
static inline void INIT_LIST_HEAD(list_head_t *list) {
    list->prev = list;
    list->next = list;
}

/**
 * @def list_entry(p, type, member)
 * @brief Retrieves the enclosing structure for a list node.
 *
 * @param p Pointer to the list node.
 * @param type The type of the enclosing structure.
 * @param member The name of the list node member within the structure.
 * @return Pointer to the enclosing structure.
 */
#define list_entry(p, type, member) \
    container_of(p, type, member)

/**
 * @def list_first_entry(p, type, member)
 * @brief Retrieves the first entry in the list.
 *
 * @param p Pointer to the list head.
 * @param type The type of the enclosing structure.
 * @param member The name of the list node member within the structure.
 * @return Pointer to the first entry in the list.
 */
#define list_first_entry(p, type, member) \
    list_entry((p)->next, type, member)

/**
 * @def list_last_entry(p, type, member)
 * @brief Retrieves the last entry in the list.
 *
 * @param p Pointer to the list head.
 * @param type The type of the enclosing structure.
 * @param member The name of the list node member within the structure.
 * @return Pointer to the last entry in the list.
 */
#define list_last_entry(p, type, member) \
    list_entry((p)->prev, type, member)

/**
 * @def list_next_entry(p, member)
 * @brief Retrieves the next entry in the list.
 *
 * @param p Pointer to the current entry.
 * @param member The name of the list node member within the structure.
 * @return Pointer to the next entry in the list.
 */
#define list_next_entry(p, member) \
    list_entry((p)->member.next, typeof(*p), member)

/**
 * @def list_prev_entry(p, member)
 * @brief Retrieves the previous entry in the list.
 *
 * @param p Pointer to the current entry.
 * @param member The name of the list node member within the structure.
 * @return Pointer to the previous entry in the list.
 */
#define list_prev_entry(p, member) \
    list_entry((p)->member.prev, typeof(*p), member)

/**
 * @def list_foreach(pos, head, member)
 * @brief Iterates over a list.
 *
 * @param pos Pointer to the current list node.
 * @param head Pointer to the list head.
 */
#define list_foreach(pos, head) \
    for (pos = head->next; pos != head; pos = pos->next)

#define list_foreach_safe(pos, next, head) \
    for (pos = head->next, next = pos->next; pos != head; pos = next, next = next->next)

/**
 * @def list_foreach_reverse(pos, head, member)
 * @brief Iterates over a list in reverse order.
 *
 * @param pos Pointer to the current list node.
 * @param head Pointer to the list head.
 */
#define list_foreach_reverse(pos, head) \
    for (pos = head->prev; pos != head; pos = pos->prev)

#define list_foreach_reverse_safe(pos, prev, head) \
    for (pos = head->prev, prev = pos->prev; pos != head; pos = prev, prev = prev->prev)

/**
 * @def list_foreach_entry(item, head, member)
 * @brief Iterates over entries in a list.
 *
 * @param item Pointer to the current entry.
 * @param head Pointer to the list head.
 * @param member The name of the list node member within the structure.
 */
#define list_foreach_entry(item, head, member)                 \
    for (item = list_first_entry(head, typeof(*item), member); \
         &(item)->member != head;                             \
         item = list_next_entry(item, member))

/**
 * @def list_foreach_entry_reverse(item, head, member)
 * @brief Iterates over entries in a list in reverse order.
 *
 * @param item Pointer to the current entry.
 * @param head Pointer to the list head.
 * @param member The name of the list node member within the structure.
 */
#define list_foreach_entry_reverse(item, head, member)        \
    for (item = list_prev_entry(head, typeof(*item), member); \
         &(item)->member != head;                            \
         item = list_prev_entry(item, member))

/**
 * @def list_foreach_entry_safe(item, n, head, member)
 * @brief Iterates over entries in a list safely (allows deletion during iteration).
 *
 * @param item Pointer to the current entry.
 * @param n Pointer to the next entry (used for safe iteration).
 * @param head Pointer to the list head.
 * @param member The name of the list node member within the structure.
 */
#define list_foreach_entry_safe(item, n, head, member)         \
    for (item = list_first_entry(head, typeof(*item), member), \
        n = list_next_entry(item, member);                    \
         &(item)->member != head;                             \
         item = n, n = list_next_entry(n, member))

/**
 * @def list_foreach_entry_reverse_safe(item, p, head, member)
 * @brief Iterates over entries in a list in reverse order safely (allows deletion during iteration).
 *
 * @param item Pointer to the current entry.
 * @param p Pointer to the previous entry (used for safe iteration).
 * @param head Pointer to the list head.
 * @param member The name of the list node member within the structure.
 */
#define list_foreach_entry_reverse_safe(item, p, head, member) \
    for (item = list_prev_entry(head, typeof(*item), member),  \
        p = list_prev_entry(item, member);                     \
         &(item)->member != head;                              \
         item = p, p = list_prev_entry(p, member))

/**
 * @brief Checks if a list is empty.
 *
 * @param head Pointer to the list head.
 * @return 1 if the list is empty, 0 otherwise.
 */
extern int list_empty(list_head_t *head);

/**
 * @brief Internal function to add a node between two nodes.
 *
 * @param node The node to add.
 * @param prev The previous node.
 * @param next The next node.
 */
extern void __list_add(list_head_t *node, list_head_t *prev, list_head_t *next);

/**
 * @brief Adds a node to the beginning of the list.
 *
 * @param node The node to add.
 * @param head The list head.
 */
extern void list_add(list_head_t *node, list_head_t *head);

/**
 * @brief Adds a node to the end of the list.
 *
 * @param node The node to add.
 * @param head The list head.
 */
extern void list_add_tail(list_head_t *node, list_head_t *head);

/**
 * @brief Internal function to remove a node by connecting its previous and next nodes.
 *
 * @param prev The previous node.
 * @param next The next node.
 */
extern void __list_remove(list_head_t *prev, list_head_t *next);

/**
 * @brief Removes a node from the list.
 *
 * @param node The node to remove.
 */
extern void list_remove(list_head_t *node);

/**
 * @brief Removes the last node from the list.
 *
 * @param head The list head.
 */
extern void list_remove_tail(list_head_t *head);

/**
 * @brief Internal function to rellocate a node.
 *
 * @param node The node to rellocate.
 * @param prev The previous node.
 * @param next The next node.
 */
extern void __list_rellocate_node(list_head_t *node, list_head_t *prev, list_head_t *next);

/**
 * @brief Rellocates a node to the beginning of another list.
 *
 * @param node The node to rellocate.
 * @param head The destination list head.
 */
extern void list_rellocate_node(list_head_t *node, list_head_t *head);

/**
 * @brief Replaces an old node with a new node.
 *
 * @param old The node to replace.
 * @param new The new node.
 */
extern void list_replace(list_head_t *old, list_head_t *new);

/**
 * @brief Extracts a slice of the list from start to end (exclusive) and returns it as a new list.
 *
 * @param head The list head.
 * @param start The starting node of the slice.
 * @param end The ending node of the slice (exclusive).
 * @return Pointer to the new list containing the slice.
 */
extern list_head_t* list_slice(list_head_t *head, list_head_t *start, list_head_t *end);

/**
 * @brief Merges two lists (list2 is merged into list1, and list2 becomes empty).
 *
 * @param list1 The destination list.
 * @param list2 The source list.
 */
extern void list_merge(list_head_t *list1, list_head_t *list2);

/**
 * @brief Extracts a slice from src_list and merges it into dest_list.
 *
 * @param src_list The source list.
 * @param dest_list The destination list.
 * @param start The starting node of the slice.
 * @param end The ending node of the slice (exclusive).
 */
extern void list_slice_and_merge(list_head_t *src_list, list_head_t *dest_list, list_head_t *start, list_head_t *end);

/**
 * @brief Calculates the length of a list.
 *
 * @param head Pointer to the list head.
 * @return The number of nodes in the list.
 */
extern usize list_length(list_head_t *head);


typedef enum {
    LIST_RELLOC_HEAD, // Relative to the head of the list
    LIST_RELLOC_TAIL  // Relative to the tail of the list
} list_relloc_t;

/**
 * @brief Migrates a slice of nodes from one list to another.
 *
 * @param dst Destination list where nodes will be moved.
 * @param src Source list from which nodes will be taken.
 * @param start_pos Starting position in the source list.
 * @param num_nodes Number of nodes to migrate.
 * @param whence Reference point for start_pos (head or tail).
 * @return 0 on success, -EINVAL on failure (e.g., invalid parameters).
 */
extern int list_migrate_range(list_head_t *dst, list_head_t *src, usize start_pos, usize num_nodes, list_relloc_t whence);