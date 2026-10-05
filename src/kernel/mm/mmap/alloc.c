#include <bits/errno.h>
#include <core/defs.h>
#include <mm/mmap/mmap.h>

int mmap_alloc_range(mmap_t *mmap, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pvmregion_node) {
    if (mmap == NULL || pvmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    bool is_user = (mmap->flags & MmapUser) ? true : false;
    flags |= (vmregion_flags_t)(is_user ? VmregionUser : 0);

    int err = __vmregion_check_flags(flags);
    if (err) { return err; }

    // Fixed mappings replace overlaps; other mappings use the nearest free region.
    if (__vmregion_flags_fixed(flags)) { // remove any existing region(s)
        return mmap_remove_range_and_set_flags(mmap, start, len, flags, pvmregion_node);
    }

    return mmap_take_nearest_slice(mmap, start, len, flags, pvmregion_node);
}

int mmap_alloc_range_as_stack(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pvmregion_node) {
    mmap_assert_locked(mmap);
    // Stack regions grow downward, so placement follows stack-specific rules.
    return mmap_alloc_range(mmap, start, len, VmregionStack, pvmregion_node);
}

int mmap_alloc_stack(mmap_t *mmap, size_t stack_size, vmregion_node_t **pvmregion_node) {
    mmap_assert_locked(mmap);
    return mmap_alloc_range(mmap, MM_LIMIT - stack_size, stack_size, VmregionStack, pvmregion_node);
}

int mmap_free_range(mmap_t *mmap, uintptr_t start, size_t len) {
    if (mmap == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);
    return mmap_remove_range(mmap, start, len, NULL);
}

