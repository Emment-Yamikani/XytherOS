#include <bits/errno.h>
#include <core/debug.h>
#include <core/defs.h>
#include <mm/kalloc.h>
#include <mm/mmap/mmap.h>

static int __vmregion_node_alloc(vmregion_node_t **pvmregion_node) {
    if (pvmregion_node == NULL) {
        return -EINVAL;
    }

    vmregion_node_t *vmregion_node = kzalloc(sizeof *vmregion_node);
    if (vmregion_node == NULL) {
        return -ENOMEM;
    }

    int err = qnode_init(&vmregion_node->list_node, vmregion_node);
    if (err) {
        kfree(vmregion_node);
        return err;
    }

    *pvmregion_node = vmregion_node;
    return 0;
}

int vmregion_node_create(vmregion_t *vmregion, mmap_t *mmap, vmregion_node_t **pnode) {
    if (vmregion == NULL || mmap == NULL || pnode == NULL) {
        return -EINVAL;
    }

    vmregion_node_t *vmregion_node;
    int err = __vmregion_node_alloc(&vmregion_node);
    if (err) { return err; }

    vmregion_node->mmap     = mmap;
    vmregion_node->vmregion = __vmregion_getref(vmregion);

    *pnode = vmregion_node;
    return 0;
}

int vmregion_node_create_from(uintptr_t start, size_t len, vmregion_flags_t flags, mmap_t *mmap, vmregion_node_t **pvmregion_node) {
    if (mmap == NULL || pvmregion_node == NULL) {
        return -EINVAL;
    }

    vmregion_t *vmregion;
    int err = vmregion_create(start, len, 0, flags, &vmregion);
    if (err != 0) { return err; }

    if ((err = vmregion_node_create(vmregion, mmap, pvmregion_node))) {
        vmregion_drop(vmregion);
        return err;
    }

    vmregion_drop(vmregion);

    return 0;
}

int vmregion_node_clone_from(vmregion_t *vmregion, mmap_t *mmap, vmregion_node_t **pvmregion_node) {
    if (vmregion == NULL || mmap == NULL || pvmregion_node == NULL) {
        return -EINVAL;
    }

    vmregion_node_t *vmregion_node;
    int err = __vmregion_node_alloc(&vmregion_node);
    if (err) { return err; }

    vmregion_t *clone_vmregion;
    if ((err = vmregion_clone(vmregion, &clone_vmregion))) {
        kfree(vmregion_node);
        return err;
    }

    vmregion_node->mmap     = mmap;
    vmregion_node->vmregion = clone_vmregion;

    *pvmregion_node = vmregion_node;
    return 0;
}

int vmregion_node_fork(vmregion_node_t *parent_node, mmap_t *child_mmap, vmregion_node_t **pchild_node) {
    if (parent_node == NULL || child_mmap == NULL || pchild_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(child_mmap);

    // The child gets its own list node but initially shares the parent's region metadata.
    vmregion_node_t *child_vmregion_node;
    vmregion_t *parent_vmregion = vmregion_from_vmregion_node(parent_node);
    int err = vmregion_node_create(parent_vmregion, child_mmap, &child_vmregion_node);
    if (err) { return err; }

    if ((err = mmap_insert_vmregion_node(child_mmap, child_vmregion_node))) {
        vmregion_node_destroy(child_vmregion_node);
        return err;
    }

    // TODO: do CoW here at the page-level.

    *pchild_node = child_vmregion_node;
    return 0;
}

int vmregion_node_remove(vmregion_node_t *vmregion_node) {
    if (vmregion_node == NULL) {
        return -EINVAL;
    }

    if (vmregion_node_is_singular(vmregion_node)) {
        return 0;
    }

    int err = qnode_remove(&vmregion_node->list_node);
    if (err) { return err; }

    if (vmregion_node->mmap) {
        mmap_assert_locked(vmregion_node->mmap); // ensure the overlaying mmap is locked.

        vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);

        // account for this space we are removing if it was used.
        if (vmregion && __vmregion_is_used(vmregion)) {
            mmap_decrement_used_size(vmregion_node->mmap, __vmregion_size(vmregion));
        }

        vmregion_node->mmap->list_length -= 1;
        vmregion_node->mmap = NULL;
    }

    return 0;
}

void vmregion_node_destroy(vmregion_node_t *vmregion_node) {
    vmregion_node_assert(vmregion_node);

    int err = vmregion_node_remove(vmregion_node);
    assert_eq(err, 0, "Errno(%d): Failed to remove vmregion_node from list.\n", err);

    if (vmregion_from_vmregion_node(vmregion_node)) {
        vmregion_drop(vmregion_from_vmregion_node(vmregion_node));
    }

    kfree(vmregion_node);
}


static inline void mmap_increment_length(mmap_t *mmap, size_t addend) {
    mmap_assert_locked(mmap);
    if (mmap) { mmap->list_length += addend; }
}

int vmregion_splice(vmregion_node_t *into, vmregion_node_t *first) {
    if (first == NULL || into == NULL) {
        return -EINVAL;
    }

    size_t used_size = 0;
    const size_t list_length = qnode_length(&first->list_node) + 1; // plus 1 to account for the first vmregion.

    // in the loop below, first will be skipped so account for it here.
    if (__vmregion_is_used(vmregion_from_vmregion_node(first))) {
        used_size += __vmregion_size(vmregion_from_vmregion_node(first));
    }

    vmregion_node_t *vmregion_node;
    // get the size of the used vmregions from the new list.
    qnode_foreach_entry(&first->list_node, vmregion_node, list_node) {
        vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
        if (__vmregion_is_used(vmregion)) {
            used_size += __vmregion_size(vmregion);
        }
    }
    
    int err = qnode_splice(&into->list_node, &first->list_node);
    if (err) { return err; }
    
    mmap_increment_length(into->mmap, list_length);
    mmap_increment_used_size(into->mmap, used_size);


    return 0;
}

int vmregion_insert_after(vmregion_node_t *vmregion_node, vmregion_node_t *prev) {
    if (vmregion_node == NULL || prev == NULL) {
        return -EINVAL;
    }

    int err = qnode_enqueue_after(&prev->list_node, &vmregion_node->list_node, QUEUE_UNIQUE);
    if (err) { return err; }

    mmap_increment_length(vmregion_node->mmap, 1);

    vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
    if (__vmregion_is_used(vmregion)) {
        mmap_increment_used_size(vmregion_node->mmap, __vmregion_size(vmregion));
    }

    return 0;
}

int vmregion_insert_before(vmregion_node_t *vmregion_node, vmregion_node_t *next) {
    if (vmregion_node == NULL || next == NULL) {
        return -EINVAL;
    }

    int err = qnode_enqueue_before(&next->list_node, &vmregion_node->list_node, QUEUE_UNIQUE);
    if (err) { return err; }

    mmap_increment_length(vmregion_node->mmap, 1);

    vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
    if (__vmregion_is_used(vmregion)) {
        mmap_increment_used_size(vmregion_node->mmap, __vmregion_size(vmregion));
    }

    return 0;
}

int vmregion_insert_between(vmregion_node_t *prev, vmregion_node_t *vmregion_node, vmregion_node_t *next) {
    if (prev == NULL || vmregion_node == NULL || next == NULL) {
        return -EINVAL;
    }

    int err = qnode_enqueue_between(&prev->list_node, &vmregion_node->list_node, &next->list_node);
    if (err) { return err; }

    mmap_increment_length(vmregion_node->mmap, 1);

    vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
    if (__vmregion_is_used(vmregion)) {
        mmap_increment_used_size(vmregion_node->mmap, __vmregion_size(vmregion));
    }

    return 0;
}

int vmregion_node_ensure_private(vmregion_node_t *vmregion_node) {
    if (vmregion_node == NULL) {
        return -EINVAL;
    }

    // Clone shared metadata before a caller changes bounds or flags in place.
    vmregion_t *vmregion = vmregion_node->vmregion;
    if (__vmregion_refcnt(vmregion) == 1) {
        return 0; // this vmregion_node is the only one having a reference to the underlaying vmregion.
    }

    vmregion_t *clone_vmregion;
    int err = vmregion_clone(vmregion, &clone_vmregion);
    if (err) { return err; }
    
    vmregion_drop(vmregion); // drop reference to the shared vmregion.

    vmregion_node->vmregion = clone_vmregion; // vmregion_node now has a unique reference.

    // mask out the VmregionShared flag, since this clone_vmregion is not shared.
    __vmregion_mask_flags(clone_vmregion, VmregionShared);

    return 0;
}

mmap_t *vmregion_node_get_mmap(vmregion_node_t *vmregion_node) {
    return vmregion_node->mmap;
}

bool vmregion_node_is_singular(vmregion_node_t *vmregion_node) {
    if (vmregion_node == NULL) {
        return false;
    }

    return qnode_empty_list(&vmregion_node->list_node);
}