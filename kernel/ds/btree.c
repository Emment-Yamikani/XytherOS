#include <bits/errno.h>
#include <ds/btree.h>
#include <mm/kalloc.h>
#include <string.h>

int btree_alloc(btree_t **pbtree) {
    int err = 0;
    btree_t *btree = NULL;
    if (pbtree == NULL)
        return -EINVAL;
    err = -ENOMEM;
    if ((btree = kmalloc(sizeof *btree)) == NULL)
        goto error;
    memset(btree, 0, sizeof *btree);
    btree->lock = SPINLOCK_INIT();
    *pbtree = btree;
    return 0;
error:
    if (btree) kfree(btree);
    return err;
}

void btree_free(btree_t *btree) {
    if (btree == NULL)
        return;
    *btree = (btree_t){0};
    kfree(btree);
}

btree_node_t *btree_alloc_node(void) {
    btree_node_t *node = NULL;
    if ((node = kmalloc(sizeof *node)) == NULL)
        return NULL;
    memset(node, 0, sizeof *node);
    return node;
}

void btree_free_node(btree_node_t *node) {
    if (node == NULL)
        return;
    memset(node, 0, sizeof *node);
    kfree(node);
    //printf("%s()\n", __func__);
}

static int btree_insert_node(btree_t *btree, btree_node_t *node) {
    btree_node_t *iter = NULL;

    if (btree == NULL || node == NULL)
        return -EINVAL;

    btree_assert_locked(btree);

    node->parent = NULL;

    if (btree_isempty(btree)) {
        btree->root = node;
        goto done;
    }

    forlinked(parent, btree->root, iter) {
        if (node->key < parent->key)
            iter = parent->left;
        else if (node->key > parent->key)
            iter = parent->right;
        else
            return -EEXIST;

        if (iter == NULL) {
            node->parent = parent;
            if (node->key < parent->key)
                parent->left = node;
            else if (node->key > parent->key)
                parent->right = node;
        }
    }

done:
    btree->nr_nodes++;
    node->btree = btree;
    return 0;
}

static void btree_delete_node(btree_t *btree, btree_node_t *node) {
    btree_node_t *parent = NULL, *left = NULL, *right = NULL;

    if (btree == NULL)
        return;

    btree_assert_locked(btree);

    if (node == NULL)
        return;

    left = node->left;
    right = node->right;
    parent = node->parent;

    if (node == btree->root)
        btree->root = NULL;
    else if (btree_node_isright(node))
        parent->right = NULL;
    else if (btree_node_isleft(node))
        parent->left = NULL;

    btree_free_node(node);
    btree->nr_nodes--;

    if (right) {
        btree->nr_nodes--;
        btree_insert_node(btree, right);
    }

    if (left) {
        btree->nr_nodes--;
        btree_insert_node(btree, left);
    }
}

btree_node_t *btree_least_node(btree_t *btree) {
    btree_node_t *node = NULL;

    if (btree == NULL)
        return NULL;

    btree_assert_locked(btree);
    forlinked(tmp, btree->root, tmp->left)
        node = tmp;
    return node;
}

btree_node_t *btree_largest_node(btree_t *btree) {
    btree_node_t *node = NULL;

    if (btree == NULL)
        return NULL;

    btree_assert_locked(btree);
    forlinked(tmp, btree->root, tmp->right)
        node = tmp;
    return node;
}

void btree_delete(btree_t *btree, btree_key_t key) {
    btree_node_t *node = NULL;
    btree_assert_locked(btree);
    node = btree_lookup(btree, key);
    btree_delete_node(btree, node);
}

int btree_search(btree_t *btree, btree_key_t key, void **pdata) {
    btree_assert_locked(btree);
    btree_node_t *node = btree_lookup(btree, key);
    if (node == NULL) return -ENOENT;
    if (pdata) *pdata = node->data;
    return 0;
}

btree_node_t *btree_lookup(btree_t *btree, btree_key_t key) {
    if (btree == NULL)
        return NULL;

    btree_assert_locked(btree);

    forlinked(node, btree->root, node) {
        if (node->key == key)
            return node;
        else if (key < node->key)
            node = node->left;
        else
            node = node->right;
    }

    return NULL;
}

int btree_insert(btree_t *btree, btree_key_t key, void *data) {
    int err = 0;
    btree_node_t *node = NULL;

    if (btree == NULL)
        return -EINVAL;

    btree_assert_locked(btree);

    if ((node = btree_alloc_node()) == NULL)
        return -ENOMEM;

    node->key = key;
    node->data = data;
    
    if ((err = btree_insert_node(btree, node)))
        goto error;
    
    return 0;
error:
    btree_free_node(node);
    return err;
}

void *btree_least(btree_t *btree) {
    btree_node_t *node = NULL;

    if (btree == NULL)
        return NULL;
    node = btree_least_node(btree);
    if (node == NULL)
        return NULL;
    return node->data;
}

void *btree_largest(btree_t *btree) {
    btree_node_t *node = NULL;

    if (btree == NULL)
        return NULL;
    node = btree_largest_node(btree);
    if (node == NULL)
        return NULL;
    return node->data;
}

int btree_node_traverse(btree_node_t *node, queue_t *queue) {
    int err = 0;

    queue_assert_locked(queue);
    if (queue == NULL)
        return -EINVAL;
    
    if (node) {
        btree_assert_locked(node->btree);
        btree_node_traverse(node->left, queue);
        if ((err = enqueue(queue, node->data, 1, NULL)))
            return err;
        btree_node_traverse(node->right, queue);
    }

    return 0;
}

int btree_traverse(btree_t *btree, queue_t *queue) {
    btree_assert_locked(btree);
    queue_assert_locked(queue);
    if (btree == NULL || queue == NULL)
        return -EINVAL;
    return btree_node_traverse(btree->root, queue);
}

int btree_flush_node(btree_node_t *node) {
    btree_node_t *right = NULL;
    if (node) {
        btree_assert_locked(node->btree);
        btree_flush_node(node->left);
        right = node->right;
        btree_delete_node(node->btree, node);
        btree_flush_node(right);
    }

    return 0;
}

int btree_flush(btree_t *btree) {
    btree_assert_locked(btree);
    if (btree == NULL)
        return -EINVAL;
    return btree_flush_node(btree->root);
}