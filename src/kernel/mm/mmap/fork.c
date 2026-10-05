#include <bits/errno.h>
#include <core/defs.h>
#include <mm/mmap/mmap.h>
#include "internals.h"

int mmap_copy(mmap_t *dest_mmap, mmap_t *src_mmap) {
    if (dest_mmap == NULL || src_mmap == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(dest_mmap);
    mmap_assert_locked(src_mmap);

    dest_mmap->brk      = src_mmap->brk;
    dest_mmap->priv     = src_mmap->priv; // TODO: handle lifetime of priv properly here.
    dest_mmap->guard    = src_mmap->guard;
    dest_mmap->flags    = src_mmap->flags;
    dest_mmap->entry    = src_mmap->entry;

    // Copy the region descriptors first; their backing metadata may remain shared.
    vmregion_node_t *vmregion_node;
    mmap_foreach_vmregion_node(src_mmap, vmregion_node) {
        vmregion_node_t *dest_vmregion_node;
        int err = vmregion_node_fork(vmregion_node, dest_mmap, &dest_vmregion_node);
        if (err != 0) { return err; }
    }

    // Delegate page-table copying to the architecture's lazy-copy implementation.
    int err = arch_lazycpy(dest_mmap->pdbr, src_mmap->pdbr);
    if (err != 0) { return err; }

    return 0;
}

int mmap_fork(mmap_t *parent, mmap_t **pchild) {
    if (parent == NULL || pchild == NULL) {
        return -EINVAL;
    }

    mmap_t *child_mmap;
    int err = mmap_alloc(MmapUser, &child_mmap);
    if (err) { return err; }

    if ((err = mmap_copy(child_mmap, parent))) {
        mmap_drop(child_mmap);
        return err;
    }

    *pchild = child_mmap;
    return 0;
}