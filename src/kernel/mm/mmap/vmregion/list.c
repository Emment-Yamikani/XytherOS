#include <bits/errno.h>
#include <core/defs.h>
#include <mm/mmap/mmap.h>

int vmregion_get_next(vmregion_node_t *vmregion_node, vmregion_node_t **pnext) {
    if (vmregion_node == NULL || pnext == NULL) {
        return -EINVAL;
    }

    *pnext = NULL;

    if (vmregion_node->list_node.next == NULL) {
        return -ENOENT;
    }

    if (vmregion_node_is_singular(vmregion_node)) {
        return -ENOENT;
    }

    if ((vmregion_node->list_node.next->data == NULL)) {
        return -ENOENT;
    }

    *pnext = vmregion_node_from_qnode(vmregion_node->list_node.next);
    return 0;
}

int vmregion_get_prev(vmregion_node_t *vmregion_node, vmregion_node_t **pprev) {
    if (vmregion_node == NULL || pprev == NULL) {
        return -EINVAL;
    }

    *pprev = NULL;

    if (vmregion_node->list_node.prev == NULL) {
        return -ENOENT;
    }

    if (vmregion_node_is_singular(vmregion_node)) {
        return -ENOENT;
    }

    if ((vmregion_node->list_node.prev->data == NULL)) {
        return -ENOENT;
    }

    *pprev = vmregion_node_from_qnode(vmregion_node->list_node.prev);
    return 0;
}

int vmregion_get_next_if_free(vmregion_node_t *vmregion_node, vmregion_node_t **pnext) {
    if (vmregion_node == NULL || pnext == NULL) {
        return -EINVAL;
    }

    *pnext = NULL;

    // Adjacency helpers deliberately skip allocated neighbors.
    vmregion_node_t *next_vmregion_node = NULL;
    int err = vmregion_get_next(vmregion_node, &next_vmregion_node);
    if (err) { return err; }

    if (!__vmregion_is_free(vmregion_from_vmregion_node(next_vmregion_node))) {
        return -ENOENT;
    }

    *pnext = next_vmregion_node;
    return 0;
}

int vmregion_get_prev_if_free(vmregion_node_t *vmregion_node, vmregion_node_t **pprev) {
    if (vmregion_node == NULL || pprev == NULL) {
        return -EINVAL;
    }

    *pprev = NULL;

    vmregion_node_t *prev_vmregion_node = NULL;
    int err = vmregion_get_prev(vmregion_node, &prev_vmregion_node);
    if (err) { return err; }

    if (!__vmregion_is_free(vmregion_from_vmregion_node(prev_vmregion_node))) {
        return -ENOENT;
    }

    *pprev = prev_vmregion_node;
    return 0;
}
