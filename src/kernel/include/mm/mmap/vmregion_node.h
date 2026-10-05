#pragma once

#include <core/assert.h>
#include <ds/internal_queue.h>
#include <mm/types.h>

typedef struct {
    mmap_t      *mmap;
    vmregion_t  *vmregion;
    qnode_t     list_node;
} vmregion_node_t;

#define vmregion_node_assert(vmregion_node)  assert(vmregion_node, "Invalid pointer to vmregion_node.\n")

#define vmregion_node_from_qnode(qnode) qnode_container(qnode, vmregion_node_t, list_node)

extern mmap_t *vmregion_node_get_mmap(vmregion_node_t *vmregion_node);

extern void vmregion_node_destroy(vmregion_node_t *vmregion_node);
extern int  vmregion_node_create(vmregion_t *vmregion, mmap_t *mmap, vmregion_node_t **pnode);
extern int  vmregion_node_create_from(uintptr_t start, size_t len, vmregion_flags_t flags, mmap_t *mmap, vmregion_node_t **pvmregion_node);

extern int  vmregion_node_clone_from(vmregion_t *vmregion, mmap_t *mmap, vmregion_node_t **pvmregion_node);

extern int  vmregion_node_ensure_private(vmregion_node_t *vmregion_node);

extern bool vmregion_node_is_singular(vmregion_node_t *vmregion_node);

extern int  vmregion_node_remove(vmregion_node_t *vmregion_node);

extern int  vmregion_insert_after(vmregion_node_t *vmregion_node, vmregion_node_t *prev);
extern int  vmregion_insert_before(vmregion_node_t *vmregion_node, vmregion_node_t *next);
extern int  vmregion_insert_between(vmregion_node_t *prev, vmregion_node_t *vmregion_node, vmregion_node_t *next);
extern int  vmregion_splice(vmregion_node_t *into, vmregion_node_t *list);

extern int  vmregion_get_next(vmregion_node_t *vmregion_node, vmregion_node_t **pnext);
extern int  vmregion_get_prev(vmregion_node_t *vmregion_node, vmregion_node_t **pprev);
extern int  vmregion_get_next_if_free(vmregion_node_t *vmregion_node, vmregion_node_t **pnext);
extern int  vmregion_get_prev_if_free(vmregion_node_t *vmregion_node, vmregion_node_t **pprev);

extern int  vmregion_take_slice_and_set_flags(vmregion_node_t *vmregion, uintptr_t start, size_t len, vmregion_flags_t flags, vmregion_node_t **pslice);

extern int vmregion_node_fork(vmregion_node_t *parent_node, mmap_t *child_mmap, vmregion_node_t **pchild_node);