#include <bits/errno.h>
#include <core/defs.h>
#include <mm/mmap/mmap.h>

static int vmregion_node_get_adjacent_free(vmregion_node_t *vmregion_node, vmregion_node_t **padjacent_node) {
    if (vmregion_node == NULL || padjacent_node == NULL) {
        return -EINVAL;
    }

    vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
    const bool grows_down = __vmregion_grows_down(vmregion);

    // Growth consumes free space on the side where this region expands.
    int (*get_adjacent)(vmregion_node_t *, vmregion_node_t **);
    get_adjacent = grows_down ? vmregion_get_prev_if_free : vmregion_get_next_if_free;

    return get_adjacent(vmregion_node, padjacent_node);
}

static int vmregion_node_can_expand(vmregion_node_t *vmregion_node, size_t size, vmregion_node_t **padjacent_node) {
    if (vmregion_node == NULL || size == 0) {
        return -EINVAL;
    }

    vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
    if (__vmregion_is_free(vmregion)) {
        return -EINVAL;
    }

    if (__vmregion_dont_expand(vmregion)) {
        return -EINVAL;
    }

    vmregion_node_t *adjacent_node;
    int err = vmregion_node_get_adjacent_free(vmregion_node, &adjacent_node);
    if (err != 0) { return err; }

    vmregion_t *adjacent = vmregion_from_vmregion_node(adjacent_node);
    if (__vmregion_size(adjacent) < size) {
        return ENOMEM;
    }

    *padjacent_node = adjacent_node;
    return 0;
}

static int vmreigon_apply_expansion(vmregion_node_t *vmregion_node, size_t explen, vmregion_node_t *adjacent_node) {
    if (vmregion_node ==  NULL || adjacent_node == NULL || !is_aligned4k(explen)) {
        return -EINVAL;
    }

    vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
    vmregion_t *adjacent = vmregion_from_vmregion_node(adjacent_node);

    if (__vmregion_grows_down(vmregion)) {
        vmregion->start -= explen;
        adjacent->end   -= explen;
    } else {
        vmregion->end   += explen;
        adjacent->start += explen;
    }

    if (__vmregion_size(adjacent) == 0) {
        vmregion_node_destroy(adjacent_node);
    }

    mmap_increment_used_size(vmregion_node->mmap, explen);
    return 0;
}

int vmregion_expand(vmregion_node_t *vmregion_node, size_t size) {
    if (vmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(vmregion_node->mmap);

    int err = vmregion_node_ensure_private(vmregion_node);
    if (err) { return err; }

    vmregion_node_t *adjacent_node;
    const size_t explen = ALIGN4KUP(size);
    if ((err = vmregion_node_can_expand(vmregion_node, explen, &adjacent_node))) {
        return err;
    }

    if (explen == 0) { return 0; } // No expansion needed.

    return vmreigon_apply_expansion(vmregion_node, explen, adjacent_node);
}