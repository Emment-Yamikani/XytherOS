#pragma once

#include <arch/paging.h>
#include <bits/errno.h>
#include <core/types.h>
#include <fs/inode.h>
#include <sync/atomic.h>

#include <mm/mmap/vmregion_flags.h>
#include <mm/mmap/vmregion_node.h>

typedef struct vmregion vmregion_t;

typedef struct vmregion_ops {
    int (*fault_handler)(vmregion_t *vmregion, pagefault_desc_t *fault_desc);
} vmregion_ops_t;

typedef struct mmap mmap_t;

typedef struct vmregion {
    atomic_long         refcnt;

    vmregion_flags_t    flags;

    size_t              guard;

    uintptr_t           end;
    uintptr_t           start;

    void                *priv;

    inode_t             *file;
    size_t              file_size;
    off_t               file_offset;

    vmregion_ops_t      *vmops;
} __aligned(16) vmregion_t;

#define vmregion_assert(vmr)    assert(vmr, "Invalid pointer to vmregion object.\n")

#define vmregion_from_vmregion_node(vmregion_node) ((vmregion_node)->vmregion)

#define vmregion_from_qnode(qnode) vmregion_from_vmregion_node(vmregion_node_from_qnode(qnode))

static inline bool __vmregion_executable(const vmregion_t *vmregion) {
    return __vmregion_flags_executable(vmregion->flags);
}

static inline bool __vmregion_writable(const vmregion_t *vmregion) {
    return __vmregion_flags_writable(vmregion->flags);
}

static inline bool __vmregion_readable(const vmregion_t *vmregion) {
    return __vmregion_flags_readable(vmregion->flags);
}

static inline bool __vmregion_read_only(vmregion_t * vmregion) {
    return __vmregion_flags_read_only(vmregion->flags);
}

static inline bool __vmregion_user(const vmregion_t *vmregion) {
    return __vmregion_flags_user(vmregion->flags);
}

static inline bool __vmregion_rx(const vmregion_t *vmregion) {
    return __vmregion_readable(vmregion) && __vmregion_executable(vmregion);
}

static inline bool __vmregion_rw(const vmregion_t *vmregion) {
    return __vmregion_readable(vmregion) && __vmregion_writable(vmregion);
}

static inline bool __vmregion_rwx(const vmregion_t *vmregion) {
    return __vmregion_rw(vmregion) && __vmregion_executable(vmregion);
}

static inline bool __vmregion_shared(const vmregion_t *vmregion) {
    return __vmregion_flags_shared(vmregion->flags);
}

static inline bool __vmregion_dont_expand(const vmregion_t *vmregion) {
    return __vmregion_flags_dont_expand(vmregion->flags);
}

static inline bool __vmregion_expand(const vmregion_t *vmregion) {
    return !__vmregion_dont_expand(vmregion);
}

static inline bool __vmregion_grows_down(const vmregion_t *vmregion) {
    return __vmregion_flags_grows_down(vmregion->flags);
}

static inline bool __vmregion_is_stack(const vmregion_t *vmregion) {
    return __vmregion_flags_grows_down(vmregion->flags);
}

static inline bool __vmregion_filebacked(const vmregion_t *vmregion) {
    return vmregion->file ? true : false;
}

static inline bool __vmregion_is_free(const vmregion_t *vmregion) {
    return __vmregion_flags_free(vmregion->flags);
}

static inline bool __vmregion_is_used(const vmregion_t *vmregion) {
    return !__vmregion_is_free(vmregion);
}

static inline bool __vmregion_zeroed(const vmregion_t *vmregion) {
    return __vmregion_flags_zeroed(vmregion->flags);
}

static inline bool __vmregion_rwz(const vmregion_t *vmregion) {
    return __vmregion_rw(vmregion) && __vmregion_zeroed(vmregion);
}

static inline bool __vmregion_test_flags(vmregion_t *vmregion, vmregion_flags_t flags) {
    return vmregion->flags & flags ? true : false;
}

static inline void __vmregion_mask_flags(vmregion_t *vmregion, vmregion_flags_t flags) {
    vmregion->flags &= ~flags;
}

static inline void __vmregion_set_flags(vmregion_t *vmregion, vmregion_flags_t flags) {
    if (__vmregion_check_flags(flags) == 0) {
        vmregion->flags |= flags & ~VmregionFixed;
    }
}

static inline void __vmregion_xor_flags(vmregion_t *vmregion, vmregion_flags_t flags) {
    vmregion->flags ^= flags;
}

static inline int __vmregion_reset_flags(vmregion_t *vmregion, vmregion_flags_t flags) {
    int err = __vmregion_check_flags(flags);
    if (err) { return err; }
    
    __vmregion_mask_flags(vmregion, VmregionMask);
    __vmregion_set_flags(vmregion, flags);
    
    return 0;
}

/**
 * @brief 
 * 
 * @param vmregion 
*/

static inline uintptr_t __vmregion_start(const vmregion_t *vmregion) {
    return vmregion->start;
}

static inline uintptr_t __vmregion_end(const vmregion_t *vmregion) {
    return vmregion->end;
}

static inline uintptr_t __vmregion_upper_bound(const vmregion_t *vmregion) {
    return vmregion->end + 1;
}

static inline size_t __vmregion_size(const vmregion_t *vmregion) {
    return __vmregion_upper_bound(vmregion) - __vmregion_start(vmregion);
}

static inline size_t __vmregion_file_size(const vmregion_t *vmregion) {
    return vmregion->file_size;
}

static inline off_t __vmregion_file_offset(const vmregion_t *vmregion) {
    return vmregion->file_offset;
}

static inline long __vmregion_refcnt(const vmregion_t *vmregion) {
    return atomic_load(&vmregion->refcnt);
}

static inline vmregion_t *__vmregion_getref(vmregion_t *vmregion) {
    if (atomic_fetch_add(&vmregion->refcnt, 1) <= 0) return NULL;
    return vmregion;
}

static inline unsigned __vmregion_to_pte_flags(const vmregion_t *vmregion) {
    return __vmregion_flags_to_pte_flags(vmregion->flags);
}

////////////////////////////////////////////////////////////|
////////////  ///|
////////////////////////////////////////////////////////////

extern void vmregion_display(vmregion_t *vmregion);

extern void vmregion_drop(vmregion_t *vmregion);
extern int  vmregion_create(uintptr_t addr, size_t len, size_t guard, vmregion_flags_t flags, vmregion_t **pvmr);

extern int  vmregion_clone(vmregion_t *vmregion, vmregion_t **pvmr);

extern int  vmregion_compare(const vmregion_t *v0, const vmregion_t *v1);
extern bool vmregion_overlaps_range(const vmregion_t *vmregion, uintptr_t addr, size_t len);
extern bool vmregion_contains_range(const vmregion_t *vmregion, uintptr_t addr, size_t len);
extern bool vmregion_within_range(const vmregion_t *vmregion, uintptr_t addr, size_t len);
extern bool vmregion_matches_range(const vmregion_t *vmregion, uintptr_t addr, size_t len);
extern bool vmregion_can_merge_prev(vmregion_t *vmregion, vmregion_t *prev);
extern bool vmregion_can_merge_next(vmregion_t *vmregion, vmregion_t *next);
extern bool vmregion_can_merge_both(vmregion_t *vmregion, vmregion_t *prev, vmregion_t *next);

extern int  vmregion_take_slice(vmregion_node_t *vmregion, uintptr_t start, size_t len, vmregion_node_t **pslice_vmr);

extern int  vmregion_shrink(vmregion_node_t *vmregion, uintptr_t start, size_t len, vmregion_node_t **pnext);

extern int vmregion_expand(vmregion_node_t *vmregion_node, size_t size);

// HOOKS //

extern int vmregion_map_all(vmregion_t *vmregion);