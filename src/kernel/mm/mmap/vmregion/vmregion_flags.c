#include <arch/paging.h>
#include <bits/errno.h>
#include <mm/mmap/vmregion_flags.h>


bool __vmregion_flags_readable(vmregion_flags_t flags) {
    return flags & VmregionR ? true : false;
}

bool __vmregion_flags_writable(vmregion_flags_t flags) {
    return flags & VmregionW ? true : false;
}

bool __vmregion_flags_executable(vmregion_flags_t flags) {
    return flags & VmregionX ? true : false;
}

bool __vmregion_flags_read_only(vmregion_flags_t flags) {
    return __vmregion_flags_readable(flags) && !(__vmregion_flags_writable(flags) || __vmregion_flags_executable(flags));
}

bool __vmregion_flags_grows_down(vmregion_flags_t flags) {
    return flags & VmregionGrowsDown ? true : false;
}

bool __vmregion_flags_free(vmregion_flags_t flags) {
    return flags & VmregionFree ? true : false;
}

bool __vmregion_flags_zeroed(vmregion_flags_t flags) {
    return flags & VmregionZero ? true : false;
}

bool __vmregion_flags_user(vmregion_flags_t flags) {
    return flags & VmregionUser ? true : false;
}

bool __vmregion_flags_dont_expand(vmregion_flags_t flags) {
    return flags & VmregionDontExpand ? true : false;
}

bool __vmregion_flags_fixed(vmregion_flags_t flags) {
    return flags & VmregionFixed ? true : false;
}

bool __vmregion_flags_shared(vmregion_flags_t flags) {
    return flags & VmregionShared ? true : false;
}

int __vmregion_check_flags(vmregion_flags_t flags) {
    if ((flags == VmregionNone) || (flags & ~VmregionMask)) {
        return -EINVAL;
    }

    bool grows_down = __vmregion_flags_grows_down(flags);
    bool readable   = __vmregion_flags_readable(flags);
    bool writable   = __vmregion_flags_writable(flags);
    bool executable = __vmregion_flags_executable(flags);
    bool shared     = __vmregion_flags_shared(flags);
    bool free       = __vmregion_flags_free(flags);

    // bool user       = __vmregion_flags_user(flags);
    // bool used       = __vmregion_flags_free(flags);
    // bool zeroed     = __vmregion_flags_zeroed(flags);
    // bool dont_expand= __vmregion_flags_dont_expand(flags);

    // Free regions carry no access or placement attributes.
    if (free && (flags & ~VmregionFree)) {
        return -EINVAL;
    }

    if (grows_down && (executable || !(readable && writable))) {
        return -EINVAL;
    }

    if (executable && !readable) {
        return -EINVAL;
    }

    if (writable && executable) {
        return -EINVAL;
    }

    if (grows_down && shared) {
        return -EINVAL;
    }

    
    return 0;
}

int __vmregion_check_flags_for_alloc(vmregion_flags_t flags) {
    if (flags & VmregionFree) {
        return -EINVAL;
    }

    return __vmregion_check_flags(flags);
}

vmregion_flags_t __vmregion_flags_get_state(vmregion_flags_t from, vmregion_flags_t flags) {
    return from & flags;
}

unsigned __vmregion_flags_to_pte_flags(vmregion_flags_t flags) {
    // Translate region policy into the architecture's PTE flags.
    unsigned int pte_flags = PTE_NONE;
    pte_flags |= __vmregion_flags_user(flags)       ? PTE_U : 0;
    pte_flags |= __vmregion_flags_readable(flags)   ? PTE_R : 0;
    pte_flags |= __vmregion_flags_writable(flags)   ? PTE_W : 0;
    pte_flags |= __vmregion_flags_executable(flags) ? PTE_X : 0;
    pte_flags |= __vmregion_flags_zeroed(flags)     ? PTE_ZERO : 0;
    return pte_flags;
}