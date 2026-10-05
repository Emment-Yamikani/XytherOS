#pragma once

#include "vmregion.h"
#include <sync/spinlock.h>

#define PROT_NONE   0x0
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4
#define PROT_MASK   0x7

#define PROT_R PROT_READ
#define PROT_W PROT_WRITE
#define PROT_X PROT_EXEC

#define PROT_RW     (PROT_R | PROT_W)
#define PROT_RX     (PROT_R | PROT_X)
#define PROT_RWX    (PROT_R | PROT_W | PROT_X)

static inline bool __prot_w(int prot) {
    return prot & PROT_WRITE ? true : false;
}

static inline bool __prot_x(int prot) {
    return prot & PROT_EXEC ? true : false;
}

static inline bool __prot_r(int prot) {
    return prot & PROT_READ ? true : false;
}

static inline bool __prot_read_only(int prot) {
    return __prot_r(prot) && !(prot & (PROT_WRITE | PROT_EXEC));
}

static inline bool __prot_valid(int prot) {
    if (prot & ~PROT_MASK)  {
        return false;
    }

    if (__prot_w(prot) && __prot_x(prot)) {
        return false;
    }

    return true;
}

static inline vmregion_flags_t __prot_to_vmregion_flags(int prot) {
    vmregion_flags_t vmregion_flags = VmregionNone;

    vmregion_flags |= __prot_w(prot) ? VmregionW : VmregionNone;
    vmregion_flags |= __prot_x(prot) ? VmregionX : VmregionNone;
    vmregion_flags |= __prot_r(prot) ? VmregionR : VmregionNone;

    return vmregion_flags;
}

#define MM_LIMIT (USTACK - 1)

#define MAP_PRIVATE                 0x0001
#define MAP_SHARED                  0x0002
#define MAP_DONTEXPAND              0x0004
#define MAP_ANON                    0x0008
#define MAP_ZERO                    0x0010
#define MAP_MAPIN                   0x0020
#define MAP_LOCK                    0x0040
#define MAP_USER                    0x0080
#define MAP_GROWSDOWN               0x0100
/*region is a stack*/
#define MAP_STACK                   MAP_GROWSDOWN

static inline vmregion_flags_t __map_flags_to_vmregion_flags(unsigned map_flags) {
    vmregion_flags_t vmregion_flags = VmregionNone;
    vmregion_flags |= map_flags & MAP_SHARED     ? VmregionShared     : 0;
    vmregion_flags |= map_flags & MAP_DONTEXPAND ? VmregionDontExpand : 0;
    vmregion_flags |= map_flags & MAP_ANON       ? VmregionAnon       : 0;
    vmregion_flags |= map_flags & MAP_ZERO       ? VmregionZero       : 0;
    vmregion_flags |= map_flags & MAP_MAPIN      ? VmregionMap        : 0;
    vmregion_flags |= map_flags & MAP_LOCK       ? VmregionLock       : 0;
    vmregion_flags |= map_flags & MAP_USER       ? VmregionUser       : 0;
    vmregion_flags |= map_flags & MAP_GROWSDOWN  ? VmregionGrowsDown  : 0;
    vmregion_flags |= map_flags & MAP_STACK      ? VmregionStack      : 0;
    return vmregion_flags;
}

/**
 * Map address range as given, 
 * and unmap any overlaping regions previously mapped.
*/
#define MAP_FIXED                   0x1000

typedef enum {
    /// Persisting 'Creation flags',
    /// these flags persist in the 'mmap->flags' after creation.
    MmapUser            = 1,            // Creates a User mmap.
} mmap_flags_t;

typedef struct mmap {
    long            refcnt;

    mmap_flags_t    flags;

    uintptr_t       brk;

    size_t          used_size;

    uintptr_t       pdbr;

    uintptr_t       limit;
    size_t          guard;

    void            *priv;

    void            *entry;

    qnode_t         list_head;
    long            list_length;

    spinlock_t      lock;
} __aligned(16) mmap_t;

#define mmap_assert(mmap)           ({ assert(mmap, "Invalid pointer to mmap object.\n"); })
#define mmap_lock(mmap)             ({ mmap_assert(mmap); spin_lock(&(mmap)->lock); })
#define mmap_unlock(mmap)           ({ mmap_assert(mmap); spin_unlock(&(mmap)->lock); })
#define mmap_trylock(mmap)          ({ mmap_assert(mmap); spin_trylock(&(mmap)->lock); })
#define mmap_islocked(mmap)         ({ mmap_assert(mmap); spin_islocked(&(mmap)->lock); })
#define mmap_recursive_lock(mmap)   ({ mmap_assert(mmap); spin_recursive_lock(&(mmap)->lock); })
#define mmap_assert_locked(mmap)    ({ mmap_assert(mmap); spin_assert_locked(&(mmap)->lock); })

#define mmap_foreach_vmregion_node(mmap, node) \
    mmap_assert_locked(mmap);                  \
    qnode_foreach_entry(&(mmap)->list_head, node, list_node)

#define mmap_foreach_vmregion_node_safe(mmap, node, next) \
    mmap_assert_locked(mmap);                             \
    qnode_foreach_entry_safe(&(mmap)->list_head, node, next, list_node)

#define mmap_foreach_vmregion_node_reverse(mmap, node) \
    mmap_assert_locked(mmap);                          \
    qnode_foreach_entry_reverse(&(mmap)->list_head, node, list_node)

#define mmap_foreach_vmregion_node_reverse_safe(mmap, node, prev) \
    mmap_assert_locked(mmap);                                     \
    qnode_foreach_entry_reverse_safe(&(mmap)->list_head, node, prev, list_node)

#define mmap_foreach_vmregion(mmap, vmregion)                                          \
    mmap_assert_locked(mmap);                                                          \
    for (qnode_t *qnode = (mmap)->list_head.next;                                      \
         (qnode != (&(mmap)->list_head)) && ((vmregion) = vmregion_from_qnode(qnode)); \
         qnode = qnode->next)

#define mmap_foreach_vmregion_safe(mmap, vmregion)                                     \
    mmap_assert_locked(mmap);                                                          \
    for (qnode_t *qnode = (mmap)->list_head.next, *next = qnode->next;                 \
         (qnode != (&(mmap)->list_head)) && ((vmregion) = vmregion_from_qnode(qnode)); \
         qnode = next, next = qnode->next)

#define mmap_foreach_vmregion_reverse(mmap, vmregion)                                  \
    mmap_assert_locked(mmap);                                                          \
    for (qnode_t *qnode = (mmap)->list_head.next;                                      \
         (qnode != (&(mmap)->list_head)) && ((vmregion) = vmregion_from_qnode(qnode)); \
         qnode = qnode->next)

#define mmap_foreach_vmregion__reverse_safe(mmap, vmregion)                            \
    mmap_assert_locked(mmap);                                                          \
    for (qnode_t *qnode = (mmap)->list_head.prev, *next = qnode->prev;                 \
         (qnode != (&(mmap)->list_head)) && ((vmregion) = vmregion_from_qnode(qnode)); \
         qnode = prev, prev = qnode->prev)

static inline mmap_t *__mmap_getref(mmap_t *mmap) {
    if (atomic_fetch_inc(&mmap->refcnt) <= 0) return NULL;
    else return mmap;
}

static inline long __mmap_putref(mmap_t *mmap) {
    return atomic_fetch_dec(&mmap->refcnt);
}

extern int mmap_switch_to(mmap_t *mmap, uintptr_t *old_pdbr);

extern int mmap_copy(mmap_t *dest_mmap, mmap_t *src_mmap);
extern int mmap_fork(mmap_t *parent, mmap_t **pchild);

extern void mmap_display(mmap_t *mmap);

extern void mmap_drop(mmap_t *mmap);
extern int  mmap_create(mmap_flags_t flags, mmap_t **pmmap);

extern void mmap_increment_used_size(mmap_t *mmap, size_t size);
extern void mmap_decrement_used_size(mmap_t *mmap, size_t size);

extern int mmap_into_iter(mmap_t *mmap, qnode_into_iter_fn_t into_iter);

extern int mmap_insert_vmregion_node(mmap_t *mmap, vmregion_node_t *vmregion_node);
extern int mmap_remove_vmregion_node(mmap_t *mmap, vmregion_node_t *vmregion_node, vmregion_node_t **pnext);

extern bool mmap_contains_vmregion(mmap_t *mmap, vmregion_t *vmregion);

extern vmregion_node_t *mmap_get_head_vmregion_node(mmap_t *mmap);

extern int mmap_get_first_vmregion_node(mmap_t *mmap, vmregion_node_t **pvmregion_node);
extern int mmap_get_address_container(mmap_t *mmap, uintptr_t addr, vmregion_node_t **pvmregion_node);
extern int mmap_get_stack_containing_address(mmap_t *mmap, uintptr_t addr, vmregion_node_t **pvmregion_node);
extern int mmap_get_stack_with_range(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pstack_vmr);
extern int mmap_get_vmregion_with_range(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pvmregion_node);
extern int mmap_get_free_vmregion_with_range(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pvmregion_node);
extern int mmap_get_nearest_vmregion_node(mmap_t *mmap, uintptr_t start, size_t len, bool reversed, vmregion_node_t **pvmregion_node);
extern int mmap_get_vmregion_with_range_and_flags(mmap_t *mmap, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pvmregion_node);

extern int mmap_take_slice(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pvmregion_node);
extern int mmap_take_slice_if_free(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pvmregion_node);
extern int mmap_take_nearest_slice(mmap_t *mmap, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pslice);
extern int mmap_take_slice_and_set_flags(mmap_t *mmap, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pslice);
extern int mmap_take_slice_if_free_and_set_flags(mmap_t *mmap, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pvmregion_node);

extern bool mmap_range_contains_used_vmregion(mmap_t *mmap, uintptr_t start, size_t len);

extern int mmap_free_range(mmap_t *mmap, uintptr_t start, size_t len);
extern int mmap_remove_range(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pvmregion_node);

extern int mmap_alloc_stack(mmap_t *mmap, size_t stack_size, vmregion_node_t **pvmregion_node);
extern int mmap_alloc_range_as_stack(mmap_t *mmap, uintptr_t start, size_t len, vmregion_node_t **pvmregion_node);
extern int mmap_alloc_range(mmap_t *mmap, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pvmregion_node);
extern int mmap_remove_range_and_set_flags(mmap_t *mmap, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pvmregion_node);

extern int mmap_protect_range(mmap_t *mmap, uintptr_t start, size_t len, int prot, vmregion_node_t **pvmregion_node);