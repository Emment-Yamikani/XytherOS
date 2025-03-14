#pragma once

#include <lib/printk.h>
#include <sync/spinlock.h>
#include <ds/queue.h>

#ifndef foreach
#define foreach(elem, list)                         \
    for (typeof(*list) *tmp = list,                 \
        elem = (typeof(elem))(tmp ? *tmp : NULL);   \
        elem; elem = *++tmp)
#endif // foreach

#ifndef forlinked
#define forlinked(elem, list, iter) \
    for (typeof(list) elem = list; elem; elem = iter)
#endif // forlinked

typedef uintptr_t btree_key_t;

struct btree;

typedef struct btree_node {
    btree_key_t          key;
    void                *data;
    struct btree_node   *left;
    struct btree_node   *parent;
    struct btree_node   *right;
    struct btree        *btree;
} btree_node_t;

typedef struct btree {
    btree_node_t    *root;
    size_t          nr_nodes;
    spinlock_t      lock;
} btree_t;

#define BTREE_INIT() ((btree_t){0})

#define BTREE_NEW()                 (&BTREE_INIT())
#define BTREE(name)                 btree_t *name = BTREE_NEW()

#define btree_assert(btree)         ({ assert(btree, "No Btree"); })
#define btree_lock(btree)           ({ btree_assert(btree); spin_lock(&(btree)->lock); })
#define btree_unlock(btree)         ({ btree_assert(btree); spin_unlock(&(btree)->lock); })
#define btree_islocked(btree)       ({ btree_assert(btree); spin_islocked(&(btree)->lock); })
#define btree_assert_locked(btree)  ({ btree_assert(btree); spin_assert_locked(&(btree)->lock); })

#define btree_nr_nodes(btree)       ({ btree_assert_locked(btree); ((btree)->nr_nodes); })
#define btree_isempty(btree)        ({ btree_assert_locked(btree); ((btree)->root == NULL); })

#define btree_node_isleft(node)     ({ (node)->parent ? ((node)->parent->left == (node)) ? 1 : 0 : 0; })
#define btree_node_isright(node)    ({ (node)->parent ? ((node)->parent->right == (node)) ? 1 : 0 : 0; })

void btree_free(btree_t *btree);
int btree_alloc(btree_t **pbtree);

/**
 * @brief btree_alloc_node
*/
btree_node_t *btree_alloc_node(void);

/**
 * @bbtree_free_noderief 
*/
void btree_free_node(btree_node_t *node);

/**
 * @brief btree_least
*/
btree_node_t *btree_least_node(btree_t *btree);

/**
 * @brief btree_least
*/
void *btree_least(btree_t *btree);

/**
 * @brief btree_largest
*/
btree_node_t *btree_largest_node(btree_t *btree);

/**
 * @brief btree_largest
*/
void *btree_largest(btree_t *btree);

/**
 * @bbtree_deleterief 
*/
void btree_delete(btree_t *btree, btree_key_t key);

/**
 * @brief btree_search
*/
int btree_search(btree_t *btree, btree_key_t key, void **pdata);

/**
 * @brief btree_lookup
*/
btree_node_t *btree_lookup(btree_t *btree, btree_key_t key);

/**
 * @btree_insertbrief 
*/
int btree_insert(btree_t *btree, btree_key_t key, void *data);

int btree_node_traverse(btree_node_t *tree, queue_t *queue);

int btree_traverse(btree_t *tree, queue_t *queue);

int btree_flush(btree_t *btree);
int btree_flush_node(btree_node_t *node);