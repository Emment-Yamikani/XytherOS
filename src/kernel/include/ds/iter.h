/**
 * @file iter.h
 * @brief Generic single-pass iterator backed by an internal queue.
 *
 * The `iter_t` interface provides a flexible, consumption-based iterator API.
 * Items are supplied by the user via a custom `init_cb` callback during initialization.
 * The iterator uses an internal queue, which is consumed during traversal.
 *
 * The iterator is not reusable: once fully traversed, it must be reinitialized.
 * Ownership and lifetime of items are determined by the user-defined `init_cb`.
 */

#pragma once

#include <ds/queue.h>

/**
 * @typedef iter_t
 * @brief iterator structure.
 */
typedef struct iter { queue_t queue; } iter_t;

#define ITER(__name) iter_t *__name = &(iter_t){0}

/**
 * @typedef iter_init_cb_t
 * @brief Initialization callback for populating the iterator.
 *
 * This function is called by `iter_init()` and is expected to enqueue items
 * into the iterator's internal queue using a known enqueue function.
 *
 * @param arg  User-supplied data (context pointer).
 * @param iter Iterator to populate.
 * @return 0 on success, negative error code on failure.
 */
typedef int (*iter_init_cb_t)(void *arg, iter_t *iter);

/**
 * @brief Creates a new iterator instance.
 *
 * @param riter Output pointer to the newly created iterator.
 * @return 0 on success, -ENOMEM on allocation failure.
 */
int iter_create(iter_t **riter);

/**
 * @brief Destroys an iterator and releases its resources.
 *
 * The internal queue is flushed before freeing the iterator.
 *
 * @param iter Iterator to destroy.
 */
void iter_destroy(iter_t *iter);

/**
 * @brief Initializes the iterator with user-supplied items.
 *
 * This function calls `init_cb(arg, iter)` to allow the user to populate
 * the internal queue. This must be called before iterating.
 *
 * @param iter     Iterator to initialize.
 * @param init_cb  Callback that populates the iterator.
 * @param arg      User-defined context argument for init_cb.
 * @return 0 on success, negative error code on failure.
 */
int iter_init(iter_t *iter, iter_init_cb_t init_cb, void *arg);

/**
 * @brief Retrieves the next item from the iterator (FIFO order).
 *
 * @param iter Iterator instance.
 * @param item Output pointer to receive the next item.
 * @return 0 on success, -ENOENT if no more items, or -EINVAL on error.
 */
int iter_next(iter_t *iter, void **item);

/**
 * @brief Retrieves the next item in reverse (LIFO order).
 *
 * @param iter Iterator instance.
 * @param item Output pointer to receive the next item.
 * @return 0 on success, -ENOENT if no more items, or -EINVAL on error.
 */
int iter_reverse_next(iter_t *iter, void **item);

extern int iter_peek(iter_t *iter, void **item);

extern int iter_peek_reverse(iter_t *iter, void **item);

/**
 * @def iter_foreach_item(item, iter, init_cb, arg)
 * @brief Macro to iterate forward over all items in a single pass.
 *
 * This macro consumes the iterator by repeatedly calling `iter_next()`.
 * It runs `init_cb(arg, iter)` once at the start to populate the iterator.
 *
 * Usage:
 * ```
 * iter_t *it;
 * iter_create(&it);
 * iter_foreach_item(item, it, my_init_cb, my_ctx) {
 *     // use item
 * }
 * iter_destroy(it);
 * ```
 *
 * @param item     Variable to hold the current item pointer.
 * @param iter     Iterator instance.
 * @param init_cb  Initialization callback of type iter_init_cb_t.
 * @param arg      Argument passed to the init callback.
 */
#define iter_foreach_item(item, iter, init_cb, arg)                      \
    for (int __ok = iter_init((iter), (iter_init_cb_t)(init_cb), (arg)); \
         __ok == 0 && (iter) && !iter_next((iter), (void **)&(item)); (item) = NULL)

/**
 * @def iter_foreach_item_reverse(item, iter, init_cb, arg)
 * @brief Macro to iterate in reverse over all items in a single pass.
 *
 * This macro consumes the iterator using `iter_reverse_next()`, and is
 * otherwise identical in behavior to `iter_foreach_item`.
 *
 * @param item     Variable to hold the current item pointer.
 * @param iter     Iterator instance.
 * @param init_cb  Initialization callback of type iter_init_cb_t.
 * @param arg      Argument passed to the init callback.
 */
#define iter_foreach_item_reverse(item, iter, init_cb, arg)              \
    for (int __ok = iter_init((iter), (iter_init_cb_t)(init_cb), (arg)); \
         __ok == 0 && (iter) && !iter_reverse_next((iter), (void **)&(item)); (item) = NULL)
