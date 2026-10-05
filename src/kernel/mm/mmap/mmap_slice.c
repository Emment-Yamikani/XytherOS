#include <bits/errno.h>
#include <mm/mmap/mmap.h>

int mmap_take_slice(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pslice_vmregion_node) {
    if (mmap == NULL || pslice_vmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    // A slice must be contained by one existing region; the slice helper handles splitting.
    vmregion_node_t *vmregion_node;
    int err = mmap_get_vmregion_with_range(mmap, start, len, &vmregion_node);
    if (err) { return err; }

    vmregion_node_t *slice_vmregion_node;
    if ((err = vmregion_take_slice(vmregion_node, start, len, &slice_vmregion_node))) {
        return err;
    }

    *pslice_vmregion_node = slice_vmregion_node;

    return 0;
}

int mmap_take_slice_and_set_flags(mmap_t *mmap, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pslice_vmregion_node) {
    if (mmap == NULL || pslice_vmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    int err = __vmregion_check_flags(flags);
    if (err) { return err; }

    mmap_assert_locked(mmap);

    vmregion_node_t *vmregion_node;
    if ((err = mmap_get_vmregion_with_range(mmap, start, len, &vmregion_node))) {
        return err;
    }

    vmregion_node_t *slice_vmregion_node;
    if ((err = vmregion_take_slice_and_set_flags(vmregion_node, start, len, flags, &slice_vmregion_node))) {
        return err;
    }

    *pslice_vmregion_node = slice_vmregion_node;

    return 0;
}

int mmap_take_slice_if_free(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pvmr) {
    if (mmap == NULL || pvmr == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    vmregion_node_t *vmregion_node;
    int err = mmap_get_vmregion_with_range_and_flags(mmap, start, len, VmregionFree, &vmregion_node);
    if (err) { return err; }

    vmregion_node_t *slice_vmregion_node;
    if ((err = vmregion_take_slice(vmregion_node, start, len, &slice_vmregion_node))) {
        return err;
    }

    *pvmr = slice_vmregion_node;

    return 0;
}

int mmap_take_slice_if_free_and_set_flags(mmap_t *mmap, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pslice) {
    if (mmap == NULL || pslice == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    int err = __vmregion_check_flags_for_alloc(flags);
    if (err) { return err; }

    vmregion_node_t *slice_vmregion_node;
    if ((err = mmap_take_slice_if_free(mmap, start, len, &slice_vmregion_node))) {
        return err;
    }

    vmregion_t *slice_vmregion = vmregion_from_vmregion_node(slice_vmregion_node);
    if ((err = __vmregion_reset_flags(slice_vmregion, flags))) {
        return err;
    }

    *pslice = slice_vmregion_node;
    return 0;
}

int mmap_take_nearest_slice(mmap_t *mmap, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pslice) {
    if (mmap == NULL || pslice == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    int err = __vmregion_check_flags_for_alloc(flags);
    if (err) { return err; }

    vmregion_node_t *vmregion_node;
    // Down-growing regions search from the high-address end to preserve stack placement.
    bool reversed = __vmregion_flags_grows_down(flags);
    if ((err = mmap_get_nearest_vmregion_node(mmap, start, len, reversed, &vmregion_node))) {
        return err;
    }

    vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
    uintptr_t start_addr = reversed ? __vmregion_upper_bound(vmregion) - len : __vmregion_start(vmregion);
    
    vmregion_node_t *slice_vmregion_node;
    if ((err = vmregion_take_slice(vmregion_node, start_addr, len, &slice_vmregion_node))) {
        return err;
    }

    vmregion_t *slice_vmregion = vmregion_from_vmregion_node(slice_vmregion_node);
    if ((err = __vmregion_reset_flags(slice_vmregion, flags))) {
        return err;
    }

    *pslice = slice_vmregion_node;
    return 0;
}