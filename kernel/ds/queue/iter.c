#include <bits/errno.h>
#include <ds/iter.h>
#include <ds/queue.h>
#include <mm/kalloc.h>

int iter_init(iter_t *iter, int (*init)(void *arg, iter_t *iter), void *arg) {
    if (!iter || !init) {
        return -EINVAL;
    }

    queue_lock(&iter->queue);
    int err = init(arg, iter);
    queue_unlock(&iter->queue);

    return err;
}

int iter_next(iter_t *iter, void **item) {
    if (!iter || !item) {
        return -EINVAL;
    }

    queue_lock(&iter->queue);
    int err = dequeue(&iter->queue, item);
    queue_unlock(&iter->queue);

    return err;
}

int iter_peek(iter_t *iter, void **item) {
    if (!iter || !item) {
        return -EINVAL;
    }

    queue_lock(&iter->queue);
    int err = queue_peek(&iter->queue, QUEUE_HEAD, item);
    queue_unlock(&iter->queue);

    return err;
}

int iter_peek_reverse(iter_t *iter, void **item) {
    if (!iter || !item) {
        return -EINVAL;
    }

    queue_lock(&iter->queue);
    int err = queue_peek(&iter->queue, QUEUE_TAIL, item);
    queue_unlock(&iter->queue);

    return err;
}

int iter_reverse_next(iter_t *iter, void **item) {
    if (!iter || !item) {
        return -EINVAL;
    }

    queue_lock(&iter->queue);
    int err = dequeue_tail(&iter->queue, item);
    queue_unlock(&iter->queue);

    return err;
}

void iter_destroy(iter_t *iter) {
    if (iter == NULL) {
        return;
    }

    queue_lock(&iter->queue);
    queue_flush(&iter->queue);
    queue_unlock(&iter->queue);
    kfree(iter);
}

int iter_create(iter_t **riter) {
    if (riter == NULL) {
        return -EINVAL;
    }

    iter_t *iter = kzalloc(sizeof *iter);
    if (iter == NULL) {
        return -ENOMEM;
    }

    int err = queue_init(&iter->queue);
    if (err != 0) {
        kfree(iter);
        return err;
    }

    *riter = iter;
    return 0;
}