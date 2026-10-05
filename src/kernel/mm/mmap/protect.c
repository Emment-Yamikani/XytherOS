#include <bits/errno.h>
#include <mm/mmap/mmap.h>

static int __mmap_validate_range(mmap_t *mmap, uintptr_t start, size_t len) {
    if (start == 0 || len == 0) {
        return -EINVAL;
    }

    if ((start + len) > mmap->limit) {
        return -EINVAL;
    }

    return 0;
}

int mmap_protect_range(mmap_t *mmap, uintptr_t start, size_t len, int prot, vmregion_node_t **pvmregion_node) {
    if (mmap == NULL || !__prot_valid(prot) || pvmregion_node == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    int err = __mmap_validate_range(mmap, start, len);
    if (err) { return err; }

    vmregion_flags_t new_flags = __prot_to_vmregion_flags(prot);

    vmregion_node_t *vmregion_node;
    if ((err = mmap_get_vmregion_with_range(mmap, start, len, &vmregion_node))) {
        if (err == -ENOENT) {
            return -ENOMEM;
        }

        return err;
    }

    vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
    const vmregion_flags_t current_flags = __vmregion_flags_get_state(vmregion->flags, VmregionRWX);

    // Not allowed to change access permissions for a stack region.
    if (__vmregion_is_stack(vmregion)) {
        return -EACCES;
    }

    if (__vmregion_is_free(vmregion)) {
        return -ENOMEM;
    }

    // No change needed.
    if (current_flags == new_flags) {
        return 0;
    }

    // Isolate the requested subrange so neighboring bytes retain their old permissions.
    vmregion_node_t *slice_vmregion_node;
    if ((err = vmregion_take_slice(vmregion_node, start, len, &slice_vmregion_node))) {
        return err;
    }

    vmregion_t *slice_vmregion = vmregion_from_vmregion_node(slice_vmregion_node);
    __vmregion_mask_flags(slice_vmregion, VmregionRWX);

    __vmregion_set_flags(slice_vmregion, new_flags);

    *pvmregion_node = slice_vmregion_node;

    unsigned pte_flags = __vmregion_to_pte_flags(slice_vmregion);
    // Update the PTEs after the region metadata reflects the new protection.
    if ((err = arch_mprotect(__vmregion_start(slice_vmregion), __vmregion_size(slice_vmregion), pte_flags))) {
        // TODO: reverse the slicing we did above???
        return err;
    }

    return 0;
}