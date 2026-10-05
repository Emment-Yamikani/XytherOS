#include <bits/errno.h>
#include <mm/mmap/mmap.h>

static int __mmap_remove_vmregion(mmap_t *mmap, vmregion_node_t *vmregion_node, vmregion_node_t **pnext) {
    mmap_assert_locked(mmap);

    vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
    if (!mmap_contains_vmregion(mmap, vmregion)) {
        return -ENOENT;
    }

    vmregion_node_t *prev_vmregion_node, *next_vmregion_node;
    // prev_vmregion_node and next_vmregion_node are guarantteed free if non-null.
    vmregion_get_prev_if_free(vmregion_node, &prev_vmregion_node);
    vmregion_get_next_if_free(vmregion_node, &next_vmregion_node);

    vmregion_t *prev_vmregion = NULL;
    vmregion_t *next_vmregion = NULL;

    if (prev_vmregion_node) {
        prev_vmregion = vmregion_from_vmregion_node(prev_vmregion_node);
    }
    
    if (next_vmregion_node) {
        next_vmregion = vmregion_from_vmregion_node(next_vmregion_node);
    }

    if (vmregion_can_merge_both(vmregion, prev_vmregion, next_vmregion)) {
        // Reclaim the removed span by extending the next free region across both neighbors.
        // size of removed region.
        const size_t r_size = __vmregion_size(vmregion) + __vmregion_size(prev_vmregion);

        next_vmregion->start -= r_size;

        vmregion_node_destroy(prev_vmregion_node);
        vmregion_node_destroy(vmregion_node);

        if (pnext) {
            *pnext = next_qnode_container(next_vmregion_node, list_node);
        }

        return 0;
    } else if (vmregion_can_merge_prev(vmregion, prev_vmregion)) {
        if (pnext) {
            *pnext = next_qnode_container(vmregion_node, list_node);
        }

        // size of removed region.
        const size_t r_size = __vmregion_size(vmregion);

        prev_vmregion->end   += r_size;

        vmregion_node_destroy(vmregion_node);
    } else if (vmregion_can_merge_next(vmregion, next_vmregion)) {
        if (pnext) {
            *pnext = next_qnode_container(next_vmregion_node, list_node);
        }

        // size of removed region.
        const size_t r_size = __vmregion_size(vmregion);

        next_vmregion->start -= r_size;

        vmregion_node_destroy(vmregion_node);
    } else {
        if (pnext) {
            *pnext = next_qnode_container(vmregion_node, list_node);
        }

        int err = vmregion_node_ensure_private(vmregion_node);
        if (err) { return err; }
        
        vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);

        // Reuse this descriptor as a free hole after clearing mapping-specific state.
        if (__vmregion_filebacked(vmregion)) {
            // TODO: close file backing
            vmregion->file_offset = 0;
            vmregion->file_size   = 0;
        }

        if (__vmregion_is_used(vmregion)) {
            mmap_decrement_used_size(mmap, __vmregion_size(vmregion));
        }

        vmregion->guard = 0;
        vmregion->priv  = NULL;
        vmregion->flags = VmregionFree;
    }

    return 0;
}

int mmap_remove_vmregion_node(mmap_t *mmap, vmregion_node_t *vmregion_node, vmregion_node_t **pnext) {
    if (mmap == NULL || vmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);
    return __mmap_remove_vmregion(mmap, vmregion_node, pnext);
}

static int __mmap_remove_or_shrink_vmregion(mmap_t *mmap, vmregion_node_t *vmregion_node, uintptr_t start, size_t len, vmregion_node_t **pnext) {
    if (mmap == NULL || vmregion_node == NULL || pnext == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);

    if (vmregion_overlaps_range(vmregion, start, len) == false) {
        return 0; // Skip vmregion_node if does not overlap.
    }

    if (vmregion_within_range(vmregion, start, len)) {
        return __mmap_remove_vmregion(mmap, vmregion_node, pnext);
    }

    return vmregion_shrink(vmregion_node, start, len, pnext);
}

int mmap_remove_range(mmap_t *mmap, uintptr_t addr, size_t len, vmregion_node_t **pvmr) {
    if (mmap == NULL || len == 0) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    const uintptr_t start = ALIGN4K(addr);

    // Visit every overlap: the requested range may cross multiple region nodes.
    vmregion_node_t *vmregion_node, *next_vmregion_node;
    mmap_foreach_vmregion_node_safe(mmap, vmregion_node, next_vmregion_node) {
        int err = __mmap_remove_or_shrink_vmregion(mmap, vmregion_node, start, len, &next_vmregion_node);
        if (err && err != -EALREADY) {
            return err;
        }
    }

    int err = mmap_get_vmregion_with_range(mmap, start, len, &vmregion_node);
    assert_eq(err, 0, "Error(%d): Failed to restrive vmregion_node[%#lx -> %#lx](%ld)\n", err, start, start + len - 1, len);

    if (pvmr) { *pvmr = vmregion_node; }

    return 0;
}

int mmap_remove_range_and_set_flags(mmap_t *mmap, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pvmr) {
    if (mmap == NULL || pvmr == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    int err = __vmregion_check_flags_for_alloc(flags);
    if (err) { return err; }

    vmregion_node_t *vmregion_node;
    if ((err = mmap_remove_range(mmap, start, len, &vmregion_node))) {
        return err;
    }

    vmregion_node_t *slice_vmregion_node;
    if ((err = vmregion_take_slice(vmregion_node, start, len, &slice_vmregion_node))) {
        return err;
    }

    if (((err = vmregion_node_ensure_private(slice_vmregion_node)))) {
        return err;
    }

    vmregion_t *slice_vmregion = vmregion_from_vmregion_node(slice_vmregion_node);
    if ((err = __vmregion_reset_flags(slice_vmregion, flags))) {
        return err;
    }

    *pvmr = slice_vmregion_node;
    return 0;
}