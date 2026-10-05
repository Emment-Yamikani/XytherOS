#include <bits/errno.h>
#include <core/defs.h>
#include <mm/mmap/mmap.h>

/* ============================================================
 * vmregion_take_slice helpers
 *
 * Goal:
 *   Extract a sub-range [start, start+len) from an existing
 *   vmregion and return it as its own region.
 *
 * Invariants:
 *   - Region list remains ordered and non-overlapping
 *   - File offsets remain linear
 *   - mmap used_size updated iff region was free
 * ============================================================ */

/* Update accounting when a free region becomes allocated */
static inline void vmregion_finalize_slice(vmregion_t *slice_vmregion, bool was_free, mmap_t *mmap) {
    if (mmap == NULL) {
        return;
    }
    
    if (was_free) {
        __vmregion_mask_flags(slice_vmregion, VmregionFree);
        mmap_increment_used_size(mmap, __vmregion_size(slice_vmregion));
    } else {
        /// Here we subtract because we took a slice from a region that was in use.
        /// So the slice's size is counted twice during insertion into the list(NOT GOOD).
        mmap_decrement_used_size(mmap, __vmregion_size(slice_vmregion));
    }
}

/* ============================================================
 * CASE 0: Exact match
 *
 * The requested slice covers the entire region.
 * No splitting required; reuse the existing vmregion.
 * ============================================================ */
static int vmregion_slice_exact(vmregion_node_t *vmregion_node, bool was_free, vmregion_node_t **pslice) {
    vmregion_finalize_slice(vmregion_from_vmregion_node(vmregion_node), was_free, vmregion_node->mmap);
    *pslice = vmregion_node;
    return 0;
}

/* ============================================================
 * CASE 1: Slice at left boundary
 *
 * [ SLICE | REMAINDER ]
 *
 * The left portion is extracted as a new region.
 * The original region shrinks from the front.
 * ============================================================ */
static int vmregion_slice_left(vmregion_node_t *vmregion_node, uintptr_t start, size_t len, bool was_free, vmregion_node_t **pslice) {
    vmregion_t *slice_vmregion, *vmregion = vmregion_from_vmregion_node(vmregion_node);

    vmregion_node_t *slice_vmregion_node;
    int err = vmregion_node_clone_from(vmregion, vmregion_node->mmap, &slice_vmregion_node);
    if (err) { return err; }

    slice_vmregion = vmregion_from_vmregion_node(slice_vmregion_node);

    /* Define slice_vmregion boundaries */
    slice_vmregion->start = start;
    slice_vmregion->end   = start + len - 1;

    /* Insert slice_vmregion before the original region */
    if ((err = vmregion_insert_before(slice_vmregion_node, vmregion_node))) {
        vmregion_node_destroy(slice_vmregion_node);
        return err;
    }

    /* Shrink original region */
    vmregion->start = slice_vmregion->end + 1;

    /* Adjust file offsets if file-backed */
    if (__vmregion_filebacked(vmregion)) {
        slice_vmregion->file_offset = vmregion->file_offset;
        slice_vmregion->file_size   = len;

        vmregion->file_offset += len;
        vmregion->file_size   -= len;
    }

    vmregion_finalize_slice(slice_vmregion, was_free, vmregion_node->mmap);
    *pslice = slice_vmregion_node;
    return 0;
}

/* ============================================================
 * CASE 2: Slice at right_vmregion boundary
 *
 * [ REMAINDER | SLICE ]
 *
 * The right_vmregion portion is extracted as a new region.
 * The original region shrinks from the back.
 * ============================================================ */
static int vmregion_slice_right(vmregion_node_t *vmregion_node, uintptr_t start, size_t len, bool was_free, vmregion_node_t **pslice) {
    vmregion_t *slice_vmregion, *vmregion = vmregion_from_vmregion_node(vmregion_node);
    
    vmregion_node_t *slice_vmregion_node;
    int err = vmregion_node_clone_from(vmregion, vmregion_node->mmap, &slice_vmregion_node);
    if (err) { return err; }

    slice_vmregion = vmregion_from_vmregion_node(slice_vmregion_node);

    /* Shrink original region */
    vmregion->end   = start - 1;

    /* Define slice_vmregion */
    slice_vmregion->start = start;
    slice_vmregion->end   = start + len - 1;

    /* Insert slice_vmregion after the original region */
    if ((err = vmregion_insert_after(slice_vmregion_node, vmregion_node))) { goto fail; }

    /* Adjust file offsets if file-backed */
    if (__vmregion_filebacked(vmregion)) {
        slice_vmregion->file_offset  = vmregion->file_offset + __vmregion_size(vmregion);
        slice_vmregion->file_size    = len;
        vmregion->file_size         -= len;
    }

    vmregion_finalize_slice(slice_vmregion, was_free, vmregion_node->mmap);
    *pslice = slice_vmregion_node;
    return 0;

fail:
    vmregion_node_destroy(slice_vmregion_node);
    return err;
}

/* ============================================================
 * CASE 3: Slice in the middle
 *
 * [ LEFT | SLICE | RIGHT ]
 *
 * Original region becomes LEFT
 * New regions created for SLICE and RIGHT
 * ============================================================ */
static int vmregion_slice_middle(vmregion_node_t *vmregion_node, uintptr_t start, size_t len, bool was_free, vmregion_node_t **pslice) {
    vmregion_t *slice_vmregion, *right_vmregion, *vmregion = vmregion_from_vmregion_node(vmregion_node);
    
    /* Clone original twice: one for slice, one for right_vmregion */
    vmregion_node_t *slice_vmregion_node;
    int err = vmregion_node_clone_from(vmregion, vmregion_node->mmap, &slice_vmregion_node);
    if (err) { return err; }

    slice_vmregion = vmregion_from_vmregion_node(slice_vmregion_node);

    vmregion_node_t *right_vmregion_node;
    if ((err = vmregion_node_clone_from(vmregion, vmregion_node->mmap, &right_vmregion_node))) {
        vmregion_node_destroy(slice_vmregion_node);
        return err;
    }

    right_vmregion = vmregion_from_vmregion_node(right_vmregion_node);

    uintptr_t orig_end = __vmregion_end(vmregion);

    /* LEFT: reuse original region */
    vmregion->end   = start - 1;

    /* SLICE */
    slice_vmregion->start = start;
    slice_vmregion->end   = start + len - 1;

    /* RIGHT */
    right_vmregion->start = slice_vmregion->end + 1;
    right_vmregion->end   = orig_end;

    /* Insert regions in order: LEFT → SLICE → RIGHT */
    if ((err = vmregion_insert_after(slice_vmregion_node, vmregion_node))) {
        goto fail;
    }

    if ((err = vmregion_insert_after(right_vmregion_node, slice_vmregion_node))) {
        goto fail;
    }

    /* File offsets advance linearly */
    if (__vmregion_filebacked(vmregion)) {
        size_t left_len = __vmregion_size(vmregion);

        slice_vmregion->file_offset = vmregion->file_offset + left_len;
        slice_vmregion->file_size   = len;

        right_vmregion->file_offset = slice_vmregion->file_offset + len;

        vmregion->file_size = left_len;
    }

    vmregion_finalize_slice(slice_vmregion, was_free, vmregion_node->mmap);
    *pslice = slice_vmregion_node;
    return 0;

fail:
    vmregion_node_destroy(right_vmregion_node);
    vmregion_node_destroy(slice_vmregion_node);
    return err;
}

/* ============================================================
 * vmregion_take_slice
 *
 * Public entry point.
 * Dispatches to the correct split case after validation
 * and page alignment.
 * ============================================================ */
int vmregion_take_slice(vmregion_node_t *vmregion_node, uintptr_t start, size_t len, vmregion_node_t **pslice) {
    if (!vmregion_node || !pslice) {
        return -EINVAL;
    }

    int err = vmregion_node_ensure_private(vmregion_node);
    if (err) { return err; }

    const vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
    bool was_free   = __vmregion_is_free(vmregion);

    /* Enforce page alignment */
    uintptr_t s     = ALIGN4K(start);
    size_t    l     = ALIGN4KUP(len);
    uintptr_t e     = s + l - 1;

    /* Must fit entirely inside the region */
    if (!vmregion_contains_range(vmregion, s, l)) {
        return -EINVAL;
    }

    /* Dispatch based on position */
    if (vmregion_matches_range(vmregion, s, l)) {
        return vmregion_slice_exact(vmregion_node, was_free, pslice);
    }

    if (__vmregion_start(vmregion) == s) {
        return vmregion_slice_left(vmregion_node, s, l, was_free, pslice);
    }

    if (__vmregion_end(vmregion) == e) {
        return vmregion_slice_right(vmregion_node, s, l, was_free, pslice);
    }

    return vmregion_slice_middle(vmregion_node, s, l, was_free, pslice);
}

int vmregion_take_slice_and_set_flags(vmregion_node_t *vmregion_node, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pslice) {
    if (vmregion_node == NULL || pslice == NULL) {
        return -EINVAL;
    }

    int err = __vmregion_check_flags_for_alloc(flags);
    if (err) { return err; }

    vmregion_node_t *slice_vmregion_node;
    if ((err = vmregion_take_slice(vmregion_node, start, len, &slice_vmregion_node))) {
        return err;
    }

    vmregion_t *slice_vmregion = vmregion_from_vmregion_node(slice_vmregion_node);
    if ((err = __vmregion_reset_flags(slice_vmregion, flags))) {
        return err;
    }

    *pslice = slice_vmregion_node;
    return 0;
}

int vmregion_shrink(vmregion_node_t *vmregion_node, uintptr_t start, size_t len, vmregion_node_t **pnext) {
    if (vmregion_node == NULL || pnext == NULL) {
        return -EINVAL;
    }

    vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
    if (!vmregion_overlaps_range(vmregion, start, len)) {
        return -EINVAL;
    }

    int err = vmregion_node_ensure_private(vmregion_node);
    if (err) { return err; }

    vmregion = vmregion_from_vmregion_node(vmregion_node);

    const uintptr_t vstart      = __vmregion_start(vmregion), vend = __vmregion_end(vmregion);

    const uintptr_t end         = start + len - 1;

    const uintptr_t ov_start    = start > vstart ? start : vstart;
    const uintptr_t ov_end      = end < vend ? end : vend;
    const size_t    ov_size     = ov_end - ov_start + 1;

    if (ov_start == vstart) {
        if (__vmregion_is_free(vmregion)) {
            return -EALREADY;
        }

        vmregion->start += ov_size;

        if (__vmregion_filebacked(vmregion)) {
            vmregion->file_offset += ov_size;
            vmregion->file_size   -= ov_size;
        }

        vmregion_node_t *prev_vmregion_node;
        vmregion_get_prev_if_free(vmregion_node, &prev_vmregion_node);
        if (prev_vmregion_node) {
            vmregion_from_vmregion_node(prev_vmregion_node)->end   += ov_size;
        } else {
            vmregion_t *new_vmregion;
            int err = vmregion_create(ov_start, ov_size, 0, VmregionFree, &new_vmregion);
            if (err) { return err; }

            vmregion_node_t *new_vmregion_node;
            if ((err = vmregion_node_create(new_vmregion, vmregion_node->mmap, &new_vmregion_node))) {
                vmregion_drop(new_vmregion);
                return err;
            }

            vmregion_drop(new_vmregion);

            if ((err = vmregion_insert_before(new_vmregion_node, vmregion_node))) {
                vmregion_node_destroy(new_vmregion_node);
                return err;
            }
        }

        *pnext = next_qnode_container(vmregion_node, list_node);
    } else if (ov_end == vend) {
        if (__vmregion_is_free(vmregion)) {
            return -EALREADY;
        }

        vmregion->end   -= ov_size;

        if (__vmregion_filebacked(vmregion)) {
            vmregion->file_size -= ov_size;
        }

        vmregion_node_t *next_vmregion_node;
        vmregion_get_next_if_free(vmregion_node, &next_vmregion_node);
        if (next_vmregion_node) {
            vmregion_from_vmregion_node(next_vmregion_node)->start -= ov_size;

            /**
             * Here we return next directly for easy handling.
             * this means this node is considred again and the usual operations will apply.
             * I think this is easier than handling explicitly the case where next range overlaps vmregion and next
             * in which case it becomes cumbersome to handle.*/
            *pnext = next_vmregion_node;
        } else {
            *pnext = next_qnode_container(vmregion_node, list_node);

            vmregion_t *new_vmregion;
            int err = vmregion_create(ov_start, ov_size, 0, VmregionFree, &new_vmregion);
            if (err) { return err; }

            vmregion_node_t *new_vmregion_node;
            if ((err = vmregion_node_create(new_vmregion, vmregion_node->mmap, &new_vmregion_node))) {
                vmregion_drop(new_vmregion);
                return err;
            }

            vmregion_drop(new_vmregion);

            if ((err = vmregion_insert_after(new_vmregion_node, vmregion_node))) {
                vmregion_node_destroy(new_vmregion_node);
                return err;
            }
        }

    } else {
        if (__vmregion_is_free(vmregion)) {
            return -EALREADY;
        }

        vmregion_node_t *mid_vmregion_node;
        int err = vmregion_node_create_from(ov_start, ov_size, VmregionFree, vmregion_node->mmap, &mid_vmregion_node);
        if (err) { return err; }

        vmregion_node_t *right_vmregion_node;
        if ((err = vmregion_node_clone_from(vmregion, vmregion_node->mmap, &right_vmregion_node))) {
            vmregion_node_destroy(mid_vmregion_node);
            return err;
        }

        vmregion_t *mid_vmregion = vmregion_from_vmregion_node(mid_vmregion_node);
        vmregion_t *right_vmregion = vmregion_from_vmregion_node(right_vmregion_node);

        right_vmregion->start = __vmregion_upper_bound(mid_vmregion);

        if ((err = vmregion_insert_after(right_vmregion_node, mid_vmregion_node))) {
            vmregion_node_destroy(right_vmregion_node);
            vmregion_node_destroy(mid_vmregion_node);
            return err;
        }

        if ((err = vmregion_splice(vmregion_node, mid_vmregion_node))) {
            vmregion_node_destroy(right_vmregion_node);
            vmregion_node_destroy(mid_vmregion_node);
            return err;
        }

        vmregion->end   = ov_start - 1;

        /* Here next becomes right_vmregion. */
        *pnext = right_vmregion_node;
    }

    if (vmregion_node->mmap) {
        mmap_decrement_used_size(vmregion_node->mmap, ov_size);
    }

    return 0;
}