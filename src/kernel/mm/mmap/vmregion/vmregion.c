#include <bits/errno.h>
#include <core/debug.h>
#include <core/defs.h>
#include <mm/kalloc.h>
#include <mm/mmap/mmap.h>

void vmregion_display(vmregion_t *vmregion) {
    vmregion_assert(vmregion);

    char *type = __vmregion_rx(vmregion) ? "\e[31m.text\e[0m" : __vmregion_is_stack(vmregion) ? "\e[33m.stack\e[0m"
                                                 : __vmregion_rwz(vmregion)        ? "\e[35m.bss\e[0m"
                                                 : __vmregion_rw(vmregion)         ? "\e[36m.data\e[0m"
                                                 : __vmregion_read_only(vmregion)  ? "\e[37m.rdata\e[0m"
                                                 : __vmregion_is_free(vmregion)    ? "\e[46m.hole\e[0m" : "\e[45m.resrv\e[0m";
    printk("| %-15s | \e[32m%#014lx\e[0m | \e[32m%#014lx\e[0m | \e[96m%12lu\e[0m | \e[34m%c%c%c%c\e[0m |  %-4s |  %-2ld |\n",
        type, __vmregion_start(vmregion), __vmregion_end(vmregion), __vmregion_size(vmregion) / 1024,
        __vmregion_readable(vmregion) ? 'r' : '-', __vmregion_writable(vmregion) ? 'w' : '-', __vmregion_executable(vmregion) ? 'x' : '-',
        __vmregion_user(vmregion) ? 'u' : 'k', __vmregion_shared(vmregion) ? "SHR" : "PRV", __vmregion_refcnt(vmregion)
    );
}

static int __vmregion_alloc(vmregion_t **pvmr) {
    if (pvmr == NULL) {
        return -EINVAL;
    }

    vmregion_t *vmregion = kzalloc(sizeof (vmregion_t));
    if (vmregion == NULL) {
        return -ENOMEM;
    }

    vmregion->refcnt = 1;

    *pvmr = vmregion;
    return 0;
}

int vmregion_create(uintptr_t addr, size_t len, size_t guard, vmregion_flags_t flags, vmregion_t **pvmr) {
    if (pvmr == NULL || len == 0) {
        return -EINVAL;
    }

    int err = __vmregion_check_flags(flags);
    if (err != 0) { return err; }

    vmregion_t *vmregion;
    if ((err = __vmregion_alloc(&vmregion))) {
        return err;
    }

    // Store an inclusive, page-aligned address interval for this region.
    vmregion->flags      = flags;
    vmregion->guard      = guard;
    vmregion->start      = ALIGN4K(addr);
    vmregion->end        = __vmregion_start(vmregion) + ALIGN4KUP(len) - 1;

    *pvmr = vmregion;
    return 0;
}

void vmregion_destroy(vmregion_t *vmregion) {
    vmregion_assert(vmregion);
    kfree(vmregion);
    // printk("%s:%d: Freed vmregion...\n", __FILE__, __LINE__);
}

static inline long __vmregion_putref(vmregion_t *vmregion) {
    return atomic_fetch_sub(&vmregion->refcnt, 1);
}

void vmregion_drop(vmregion_t *vmregion) {
    vmregion_assert(vmregion);
    if (__vmregion_putref(vmregion) <= 1) {
        {
            // printk("Dropping: ");
            // vmregion_display(vmregion);
        }

        vmregion_destroy(vmregion);

        {
            // printk("Dropped vmregion ref...\n");
        }
    }
}

int vmregion_clone(vmregion_t *vmregion, vmregion_t **pvmr) {
    if (vmregion == NULL || pvmr == NULL) {
        return -EINVAL;
    }

    vmregion_t *clone_vmr;
    int err = __vmregion_alloc(&clone_vmr);
    if (err) { return err; }

    clone_vmr->flags        = vmregion->flags;

    clone_vmr->end          = vmregion->end;
    clone_vmr->start        = vmregion->start;
    clone_vmr->guard        = vmregion->guard;

    clone_vmr->priv         = vmregion->priv; // TODO: Handle this shared ref

    clone_vmr->file         = vmregion->file; // TODO: file shared, handle it's lifetime.
    clone_vmr->file_size    = vmregion->file_size;
    clone_vmr->file_offset  = vmregion->file_offset;

    *pvmr = clone_vmr;
    return 0;
}

bool vmregion_overlaps_range(const vmregion_t *vmregion, uintptr_t addr, size_t len) {
    uintptr_t start = addr, end = addr + len - 1;
    if (vmregion == NULL || len == 0) {
        return false;
    }

    // Treat wrapped end calculations as extending to the top of the address space.
    if (end < addr) { end = UINTPTR_MAX; }
    return (vmregion->start <= end) && (start <= vmregion->end);
}

bool vmregion_contains_range(const vmregion_t *vmregion, uintptr_t addr, size_t len) {
    uintptr_t start = addr, end = addr + len - 1;
    if (vmregion == NULL || len == 0) {
        return false;
    }

    if (end < addr) { end = UINTPTR_MAX; }
    return (vmregion->start <= start) && (end <= vmregion->end);
}

bool vmregion_can_merge_prev(vmregion_t *vmregion, vmregion_t *prev) {
    if (!vmregion || !prev) {
        return false;
    }
    return __vmregion_upper_bound(prev) == __vmregion_start(vmregion);
}

bool vmregion_can_merge_next(vmregion_t *vmregion, vmregion_t *next) {
    if (!vmregion || !next) {
        return false;
    }
    return __vmregion_upper_bound(vmregion) == __vmregion_start(next);
}

bool vmregion_can_merge_both(vmregion_t *vmregion, vmregion_t *prev, vmregion_t *next) {
    return vmregion_can_merge_prev(vmregion, prev) && vmregion_can_merge_next(vmregion, next);
}

bool vmregion_within_range(const vmregion_t *vmregion, uintptr_t addr, size_t len) {
    uintptr_t start = addr, end = addr + len - 1;
    if (vmregion == NULL || len == 0) {
        return false;
    }

    if (end < addr) { end = UINTPTR_MAX; }
    return (start <= vmregion->start) && (vmregion->end <= end);
}

bool vmregion_matches_range(const vmregion_t *vmregion, uintptr_t addr, size_t len) {
    uintptr_t start = addr, end = addr + len - 1;
    if (vmregion == NULL || len == 0) {
        return false;
    }

    if (end < start) { end = UINTPTR_MAX; }

    return __vmregion_start(vmregion) == start && __vmregion_end(vmregion) == end;
}

int vmregion_compare(const vmregion_t *v0, const vmregion_t *v1) {
    const uintptr_t v1start = __vmregion_start(v1);
    const size_t    v1len   = __vmregion_size(v1);
    if (v0 == v1) {
        return QUEUE_EQUAL;
    } else if (vmregion_matches_range(v0, v1start, v1len)) {
        return QUEUE_EQUAL;
    } else if (__vmregion_start(v0) > __vmregion_start(v1)) {
        return QUEUE_GREATER;
    } else {
        return QUEUE_LESSER;
    }
}
