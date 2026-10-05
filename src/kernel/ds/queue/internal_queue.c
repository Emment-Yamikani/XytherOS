#include <bits/errno.h>
#include <arch/cpu.h>
#include <core/debug.h>
#include <core/assert.h>
#include <ds/internal_queue.h>
#include <mm/kalloc.h>

void qnode_to_string(qnode_t *node, char *buf) {
    snprintf(buf, 74, "(prev: %p | data: %p | next: %p)", node->prev, node->data, node->next);
}

void qnode_data_as_string(qnode_t *node, char *buf) {
    snprintf(buf, 16, "%p", node->data);
}

void qnode_print(qnode_t *head) {
    qnode_t *node;

    printk("[");
    qnode_foreach_node(head, node) {
        printk("%ld%s", (long)node->data, node->next != head ? ", " : "");
    }
    printk("]\n");
}

int qnode_into_iter(qnode_t *head, qnode_into_iter_fn_t into_iter) {
    if (head == NULL || into_iter == NULL) {
        return -EINVAL;
    }

    qnode_t *node, *next;
    qnode_foreach_node_safe(head, node, next) {
        int err = into_iter(node);
        if (err) {return err; }
    }

    return 0;
}

int qnode_iter_next(qnode_t *head, qnode_t **pnext) {
    if (head == NULL || pnext == NULL) {
        return -EINVAL;
    }

    if (head->next == head) {
        return -ENOENT;
    }

    qnode_t *next = head->next;
    int err = qnode_detach(next);
    if (err) { return err; }

    *pnext = next;
    return 0;
}

int qnode_iter_prev(qnode_t *head, qnode_t **pprev) {
    if (head == NULL || pprev == NULL) {
        return -EINVAL;
    }

    if (head->prev == head) {
        return -ENOENT;
    }

    qnode_t *prev = head->prev;
    int err = qnode_detach(prev);
    if (err) { return err; }

    *pprev = prev;
    return 0;
}

int qnode_drain(qnode_t *head) {
    if (head == NULL) {
        return -EINVAL;
    }

    qnode_t *node, *next;
    qnode_foreach_node_safe(head, node, next) {
        int err = qnode_detach(node);
        if (err) { return err; }
    }

    return 0;
}

int qnode_init(qnode_t *node, void *data) {
    if (node == NULL) {
        return -EINVAL;
    }

    node->data = data;
    node->prev = node->next = node;

    return 0;
}

int qnode_create(void *data, qnode_t **pqnode) {
    if (pqnode == NULL) {
        return -EINVAL;
    }

    qnode_t *node = kzalloc(sizeof *node);
    if (node == NULL) {
        return -ENOMEM;
    }

    int err = qnode_init(node, data);
    if (err != 0) {
        kfree(node);
        return err;
    }

    *pqnode = node;
    return 0;
}

void qnode_destroy(qnode_t *node) {
    qnode_detach(node);
    kfree(node);
}

static bool __qnode_exists(qnode_t *head, qnode_t *qnode) {
    if (!head) return 0;
    qnode_t *cur_node;
    qnode_foreach_node(head, cur_node) {
        if (cur_node == qnode || cur_node->data == qnode->data) {
            return true;
        }
    }
    return false;
}

int qnode_contains(qnode_t *head, qnode_t *qnode) {
    return __qnode_exists(head, qnode) ? 0 : -ENOENT;
}

bool qnode_empty_list(qnode_t *qnode) {
    if (qnode == NULL) { return false; }
    return (qnode->prev == qnode) && (qnode->next == qnode);
}

static void __qnode_insert_between(qnode_t *prev, qnode_t *qnode, qnode_t *next) {
    qnode_detach(qnode);

    qnode->prev = prev;
    qnode->next = next;
    prev->next  = qnode;
    next->prev  = qnode;
}

int qnode_enqueue_between(qnode_t *prev, qnode_t *node, qnode_t *next) {
    if (!prev || !node || !next) {
        return -EINVAL;
    }
    __qnode_insert_between(prev, node, next);
    return 0;
}

int qnode_enqueue(qnode_t *tail, qnode_t *qnode, queue_uniqueness_t uniq) {
    if (!tail) {
        qnode->next = qnode->prev = qnode;
        return 0;
    }

    if (uniq && __qnode_exists(tail, qnode)) return -EEXIST;

    qnode_t *head = tail->next;
    __qnode_insert_between(tail, qnode, head);
    return 0;
}

int qnode_enqueue_head(qnode_t *head, qnode_t *qnode, queue_uniqueness_t uniq) {
    if (!head) {
        qnode->next = qnode->prev = qnode;
        return 0;
    }

    if (uniq && __qnode_exists(head, qnode)) return -EEXIST;

    qnode_t *tail = head->prev;
    __qnode_insert_between(tail, qnode, head);
    return 0;
}

int qnode_enqueue_after(qnode_t *prev, qnode_t *qnode, queue_uniqueness_t uniq) {
    if (uniq && __qnode_exists(prev, qnode)) return -EEXIST;
    __qnode_insert_between(prev, qnode, prev->next);
    return 0;
}

int qnode_enqueue_before(qnode_t *next, qnode_t *qnode, queue_uniqueness_t uniq) {
    if (uniq && __qnode_exists(next, qnode)) return -EEXIST;
    __qnode_insert_between(next->prev, qnode, next);
    return 0;
}

int qnode_enqueue_whence(qnode_t *head, qnode_t *qnode, queue_uniqueness_t uniq, queue_relloc_t whence) {
    return ((whence == QUEUE_TAIL) ? qnode_enqueue_before : qnode_enqueue_after)(head, qnode, uniq);
}

int qnode_sorted_enqueue(qnode_t *head, qnode_t *qnode, queue_uniqueness_t uniq, queue_order_t order, qnode_cmp_fn_t compare) {
    /* If the list is empty, insert qnode as the sole element. */
    if (!head) {
        qnode->next = qnode->prev = qnode;
        return 0;
    }

    /* Check structural uniqueness: qnode must not be already in the list. */
    if (qnode_contains(head, qnode) == 0) {
        return -EEXIST;   // node or data already exists
    }

    qnode_t *cur_node, *next;
    qnode_foreach_node_safe(head, cur_node, next) {
        int cmp_val = compare(qnode, cur_node);

        /*
         * Uniqueness checking: If uniq == QUEUE_UNIQUE_DATA, then
         * we reject insertion if data compares as equal to any node.
         */
        if (uniq == QUEUE_UNIQUE && cmp_val == QUEUE_EQUAL) {
            return -EEXIST; 
        }

        /*
         * Determine insertion location based on sorting order.
         *
         * For ASC:   insert before first node where q <= cur_node
         * For DESC:  insert before first node where q >= cur_node
         */
        if (order == QUEUE_ASCENDING) {
            if (cmp_val == QUEUE_EQUAL || cmp_val == QUEUE_LESSER) {
                /* Insert qnode BEFORE cur_node */
                qnode->next     = cur_node;
                qnode->prev     = cur_node->prev;
                cur_node->prev->next = qnode;
                cur_node->prev       = qnode;

                return 0;
            }
        } else {  /* QUEUE_DESCENDING */
            if (cmp_val == QUEUE_EQUAL || cmp_val == QUEUE_GREATER) {
                /* Insert qnode BEFORE cur_node */
                qnode->next     = cur_node;
                qnode->prev     = cur_node->prev;
                cur_node->prev->next = qnode;
                cur_node->prev       = qnode;

                return 0;
            }
        }
    }

    /*
     * If we reach here, qnode is either:
     *   - greater than all nodes (ASC), or
     *   - less than all nodes (DESC).
     * Insert it at the tail (just before head).
     */
    qnode_t *tail = head->prev;

    qnode->next = head;
    qnode->prev = tail;
    tail->next  = qnode;
    head->prev  = qnode;

    return 0;
}

int qnode_detach(qnode_t *qnode) {
    if (qnode == NULL) return -EINVAL;

    if (qnode->next == qnode) {
        qnode->next = qnode->prev = qnode;
        return 0;
    }

    // debug("prev: %p, next: %p\n", qnode->prev, qnode->next);
    qnode->prev->next = qnode->next;
    qnode->next->prev = qnode->prev;
    qnode->next = qnode->prev = qnode;
    return 0;
}

int qnode_remove(qnode_t *node) {
    if (node == NULL) {
        return -EINVAL;
    }

    return qnode_detach(node);
}

int qnode_list_remove(qnode_t *head, qnode_t *node) {
    if (head == NULL || node == NULL) {
        return -EINVAL;
    }

    int err = qnode_contains(head, node);
    if (err) { return err; }

    return qnode_detach(node);
}

int qnode_replace(qnode_t *q0, qnode_t *q1) {
    if (!q0 || !q1) return -EINVAL;
    q1->prev = q0->prev;
    q1->next = q0->next;
    q0->prev->next = q1;
    q0->next->prev = q1;
    q0->next = q0->prev = q0;
    return 0;
}

int qnode_migrate(qnode_t *dst, qnode_t *src, usize pos, usize n, queue_relloc_t whence) {
    if (!src || !dst) return -EINVAL;

    if (n == 0) return 0;

    // find start node at offset pos
    qnode_t *start = src->next;        // <-- start at first real element
    if (start == src) return -ENOENT;  // empty source list

    for (usize i = 0; i < pos; i++) {
        start = start->next;
        if (start == src) return -EINVAL; // pos exceeds src length
    }

    // isolate n nodes as a block
    qnode_t *block_head = start;
    qnode_t *block_tail = start;

    for (usize i = 1; i < n; i++) {
        block_tail = block_tail->next;
        if (block_tail == src) return -EINVAL; // n exceeds list length
    }

    // detach block
    block_head->prev->next = block_tail->next;
    block_tail->next->prev = block_head->prev;

    // insert block into dst
    if (whence == QUEUE_HEAD) {
        qnode_t *head = dst;
        qnode_t *tail = dst->prev;
        tail->next = block_head;
        block_head->prev = tail;
        block_tail->next = head;
        head->prev = block_tail;
    } else {
        qnode_t *tail = dst;
        __unused qnode_t *head = dst->next;
        // __unused(head);
        tail->next->prev = block_tail;
        block_tail->next = tail->next;
        tail->next = block_head;
        block_head->prev = tail;
    }

    return 0;
}

int qnode_relloc(qnode_t *head, qnode_t *node, queue_uniqueness_t uniq, queue_relloc_t whence) {
    if (!node) return -EINVAL;
    return qnode_enqueue_whence(head, node, uniq, whence);
}

size_t qnode_length(qnode_t *head) {
    assert_ne(head, NULL, "head is invalid.\n");

    size_t length = 0;
    qnode_t *cur_node;
    qnode_foreach_node(head, cur_node) {
        length += 1;
    }

    return length;
}

int qnode_rotate_left(qnode_t *head) {
    assert_ne(head, NULL, "Invalid sentinel node.\n");
    if (head->prev == head || head->next == head) {
        return 0;
    }

    qnode_t *node = head->next;
    int err = qnode_detach(head);
    if (err) { return err; }

    __qnode_insert_between(node, head, node->next);
    return 0;
}

int qnode_rotate_right(qnode_t *head) {
    assert_ne(head, NULL, "Invalid sentinel node.\n");
    if (head->prev == head || head->next == head) {
        return 0;
    }

    qnode_t *node = head->prev;
    int err = qnode_detach(head);
    if (err) { return err; }

    __qnode_insert_between(node->prev, head, node);
    return 0;
}

int qnode_splice(qnode_t *list0, qnode_t *list1) {
    if (list0 == NULL || list1 == NULL) {
        return -EINVAL;
    }

    qnode_t *list0_start = list0->next;
    qnode_t *list1_end   = list1->prev;

    list0->next = list1;
    list1->prev = list0;

    list0_start->prev = list1_end;
    list1_end->next = list0_start;

    return 0;
}