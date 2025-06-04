#include <bits/errno.h>
#include <ds/btree.h>
#include <mm/kalloc.h>
#include <core/debug.h>

int btree_iter_create(btree_t *tree, iter_t **riter) {
    if (!tree || !riter) {
        return -EINVAL;
    }

    btree_assert_locked(tree);

    iter_t *iter;
    int err = iter_create(&iter);
    if (err != 0) {
        return err;
    }

    err = btree_iter_init(tree, iter);
    if (err != 0) {
        iter_destroy(iter);
        return err;
    }

    *riter = iter;
    return 0;
}

int btree_iter_init(btree_t *tree, iter_t *iter) {
    if (!tree || !iter) {
        return -EINVAL;
    }

    btree_assert_locked(tree);
    queue_lock(&iter->queue);
    int err = btree_traverse(tree, &iter->queue);
    queue_unlock(&iter->queue);
    return err;
}

int btree_iter_next(iter_t *iter, void **item) {
    if (!iter || !item) {
        return -EINVAL;
    }

    queue_lock(&iter->queue);
    int err = dequeue(&iter->queue, item);
    queue_unlock(&iter->queue);

    return err;
}

int btree_iter_reverse_next(iter_t *iter, void **item) {
    if (!iter || !item) {
        return -EINVAL;
    }

    queue_lock(&iter->queue);
    int err = dequeue_tail(&iter->queue, item);
    queue_unlock(&iter->queue);

    return err;
}

void btree_iter_destory(iter_t *iter) {
    if (iter == NULL) {
        return;
    }

    queue_lock(&iter->queue);
    queue_flush(&iter->queue);
    queue_unlock(&iter->queue);
    kfree(iter);
}