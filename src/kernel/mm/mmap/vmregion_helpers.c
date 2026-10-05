#include <bits/errno.h>
#include <mm/mmap/mmap.h>

int mmap_into_iter(mmap_t *mmap, qnode_into_iter_fn_t into_iter) {
    if (mmap == NULL || into_iter == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);
    return qnode_into_iter(&mmap->list_head, into_iter);
}

bool mmap_contains_vmregion(mmap_t *mmap, vmregion_t *vmregion) {
    if (mmap == NULL || vmregion == NULL) {
        return false;
    }

    vmregion_t *cur_vmregion;
    mmap_foreach_vmregion(mmap, cur_vmregion) {
        if (vmregion_compare(cur_vmregion, vmregion) == QUEUE_EQUAL) {
            return true;
        }
    }

    return false;
}

static int mmap_insert_sorted(mmap_t *mmap, vmregion_node_t *vmregion_node) {
    if (mmap == NULL || vmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    // Keep the list ordered by virtual address so range lookup and adjacency stay meaningful.
    vmregion_node_t *cur_vmregion_node, *next_vmregion_node;
    mmap_foreach_vmregion_node_safe(mmap, cur_vmregion_node, next_vmregion_node) {
        vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
        vmregion_t *cur_vmregion = vmregion_from_vmregion_node(cur_vmregion_node);
        switch (vmregion_compare(vmregion, cur_vmregion)) {
            case QUEUE_GREATER: continue;
            case QUEUE_EQUAL:   return -EEXIST;
            case QUEUE_LESSER:  {
                return vmregion_insert_before(vmregion_node, cur_vmregion_node);
            }
        }
    }

    /// If we reached here, vmregion is larger than all the vmregion.
    /// So just insert vmregion at the tail of the list.
    vmregion_node_t *head_node = mmap_get_head_vmregion_node(mmap);
    return vmregion_insert_before(vmregion_node, head_node);
}


int mmap_insert_vmregion_node(mmap_t *mmap, vmregion_node_t *vmregion_node) {
    if (mmap == NULL || vmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);
    return mmap_insert_sorted(mmap, vmregion_node);
}

int mmap_get_first_vmregion_node(mmap_t *mmap, vmregion_node_t **pvmregion_node) {
    if (mmap == NULL || pvmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    // Prefer a free region containing the hint, then fall back to the first large-enough hole.
    vmregion_node_t *vmregion_node;
    mmap_foreach_vmregion_node(mmap, vmregion_node) {
        *pvmregion_node = vmregion_node;
        return 0;
    }

    return -ENOENT;
}

int mmap_get_vmregion_with_range(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pvmregion_node) {
    if (mmap == NULL || pvmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    vmregion_node_t *vmregion_node;
    mmap_foreach_vmregion_node(mmap, vmregion_node) {
        vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
        if (vmregion_contains_range(vmregion, start, len)) {
            *pvmregion_node = vmregion_node;
            return 0;
        }
    }

    return -ENOENT;
}

int mmap_get_vmregion_with_range_and_flags(mmap_t *mmap, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pvmregion_node) {
    if (mmap == NULL || pvmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    vmregion_node_t *vmregion_node;
    mmap_foreach_vmregion_node(mmap, vmregion_node) {
        vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
        if (__vmregion_test_flags(vmregion, flags) == false) {
            continue;
        }

        if (vmregion_contains_range(vmregion, start, len)) {
            *pvmregion_node = vmregion_node;
            return 0;
        }
    }

    return -ENOENT;
}

int mmap_get_free_vmregion_with_range(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pfree_vmr) {
    if (mmap == NULL || pfree_vmr == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);
    return mmap_get_vmregion_with_range_and_flags(mmap, start, len, VmregionFree, pfree_vmr);
}

static int mmap_get_nearest_free_vmregion(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pvmregion_node) {
    if (mmap == NULL || pvmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    vmregion_node_t *vmregion_node;
    mmap_foreach_vmregion_node(mmap, vmregion_node) {
        vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
        if (!vmregion_contains_range(vmregion, start, len)) {
            continue;
        }

        if (__vmregion_is_free(vmregion)) {
            *pvmregion_node = vmregion_node;
            return 0;
        }
    }

    mmap_foreach_vmregion_node(mmap, vmregion_node) {
        vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
        if ((__vmregion_size(vmregion) >= len) && __vmregion_is_free(vmregion)) {
            *pvmregion_node = vmregion_node;
            return 0;
        }
    }

    return -ENOMEM;
}

static int mmap_get_nearest_free_vmregion_reversed(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pvmregion_node) {
    if (mmap == NULL || pvmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    vmregion_node_t *vmregion_node;
    mmap_foreach_vmregion_node_reverse(mmap, vmregion_node) {
        vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
        if (!vmregion_contains_range(vmregion, start, len)) {
            continue;
        }

        if (__vmregion_is_free(vmregion)) {
            *pvmregion_node = vmregion_node;
            return 0;
        }
    }

    mmap_foreach_vmregion_node_reverse(mmap, vmregion_node) {
        vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
        if ((__vmregion_size(vmregion) >= len) && __vmregion_is_free(vmregion)) {
            *pvmregion_node = vmregion_node;
            return 0;
        }
    }

    return -ENOMEM;
}

int mmap_get_nearest_vmregion_node(mmap_t *mmap, uintptr_t start, size_t len, bool reversed, vmregion_node_t **pvmregion_node) {
    if (mmap == NULL || pvmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    // Select traversal direction for ordinary mappings versus downward-growing regions.
    typedef int (*vmr_finder_fn_t)(mmap_t *, uintptr_t, size_t, vmregion_node_t **);
    vmr_finder_fn_t finder = reversed ? mmap_get_nearest_free_vmregion_reversed : mmap_get_nearest_free_vmregion;
    return finder(mmap, start, len, pvmregion_node);
}

int mmap_get_stack_with_range(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pstack_vmr) {
    if (mmap == NULL || pstack_vmr == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);
    return mmap_get_vmregion_with_range_and_flags(mmap, start, len, VmregionStack, pstack_vmr);
}

int mmap_get_address_container(mmap_t *mmap, uintptr_t addr, vmregion_node_t **pvmregion_node) {
    if (mmap == NULL || pvmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);
    return mmap_get_vmregion_with_range(mmap, ALIGN4K(addr), PGSZ, pvmregion_node);
}

int mmap_get_stack_containing_address(mmap_t *mmap, uintptr_t addr, vmregion_node_t **pvmregion_node) {
    return mmap_get_address_container(mmap, addr, pvmregion_node);
}