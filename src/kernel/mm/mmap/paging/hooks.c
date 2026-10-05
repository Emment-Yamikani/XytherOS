#include <bits/errno.h>
#include <mm/mmap/mmap.h>

int vmregion_map(vmregion_t *vmregion, uintptr_t addr, size_t len) {
    if (!vmregion || !vmregion_contains_range(vmregion, addr, len)) {
        return -EINVAL;
    }

    // Reject ranges outside this region before forwarding its permissions to the mapper.
    int vmflags = __vmregion_to_pte_flags(vmregion);
    return arch_map_n(addr, len, vmflags);
}

int vmregion_map_all(vmregion_t *vmregion) {
    if (vmregion == NULL) {
        return -EINVAL;
    }

    // Map the region's complete page-aligned span using its stored access flags.
    const uintptr_t addr  = __vmregion_start(vmregion);
    const size_t    size  = __vmregion_size(vmregion);
    const unsigned  flags = __vmregion_to_pte_flags(vmregion);

    return arch_map_n(addr, size, flags);
}

int vmregion_map_at(vmregion_t *vmregion, off_t offset, size_t len) {
    const uintptr_t addr = __vmregion_start(vmregion) + ALIGN4K(offset);
    if (!vmregion || !vmregion_contains_range(vmregion, addr, len)) {
        return -EINVAL;
    }

    return vmregion_map(vmregion, addr, len);
}

int vmregion_remap(vmregion_t *vmregion, int flags) {
    if (vmregion == NULL) {
        return -EINVAL;
    }

    __unused  int vmflags = __vmregion_to_pte_flags(vmregion);

    __unused bool prot_r = flags & 1 ? true : false;
    __unused bool prot_w = flags & 2 ? true : false;
    __unused bool prot_u = flags & 4 ? true : false;

    

    return -ENOSYS;
}

int vmregion_unmap(vmregion_t *vmregion) {
    if (vmregion == NULL) {
        return -EINVAL;
    }

    return -ENOSYS;
}
