#include <bits/errno.h>
#include <mm/kalloc.h>
#include <mm/mmap/mmap.h>

static inline int __display_qnode_into_iter(qnode_t *node) {
    vmregion_t *vmregion = vmregion_from_qnode(node);
    vmregion_display(vmregion);
    return 0;
}

void mmap_display(mmap_t *mmap) {
    mmap_assert_locked(mmap);
    printk(
        "\n ______________________________________________________________________________\n"
        "|       \e[41mMMAP DUMP\e[0m:   Owner: %s      VmrCnt: %4lu        RefCnt: %4lu         \e[0m|\n"
        "+------------------------------------------------------------------------------|\n"
        "|    Brk(\e[32m%#014lx\e[0m)  PDBR(\e[32m%#014lx\e[0m) UsedMem(\e[36m%014lu\e[0m KiB)     \e[0m|\n"
        "+--------+----------------+----------------+--------------+------+-------+-----|\n"
        "|\e[46m  %-5s \e[0m|\e[46m     %-9s  \e[0m|\e[46m      %-8s  \e[0m|\e[46m  %-11s \e[0m|\e[46m %-4s \e[0m|\e[46m %-5s \e[0m|\e[46m %3s \e[0m|\n"
        "+--------+----------------+----------------+--------------+------+-------+-----|\n",
        mmap->flags & MmapUser ? "User" : "Os", mmap->list_length, mmap->refcnt,
        mmap->brk, mmap->pdbr, mmap->used_size / 1024,
        "Type", "Start", "End", "Size(KiB)", "Prot", "Flags", "Ref"
    );
    mmap_into_iter(mmap, __display_qnode_into_iter);
    printk("+--------+----------------+----------------+--------------+------+-------+-----+\n");
}

int mmap_alloc_pdbr(mmap_t *mmap) {
    if (mmap == NULL) {
        return -EINVAL;
    }

    int err = arch_getpgdir(&mmap->pdbr);
    if (err != 0) { return err; }

    return 0;
}

int mmap_alloc(mmap_flags_t mmap_flags, mmap_t **pmmap) {
    if (pmmap == NULL || (mmap_flags == 0) || (mmap_flags & ~(MmapUser))) {
        return -EINVAL;
    }

    mmap_t *mmap = kzalloc(sizeof (mmap_t));
    if (mmap == NULL) {
        return -ENOMEM;
    }

    mmap->refcnt    = 1;
    mmap->guard     = PGSZ;
    mmap->limit     = MM_LIMIT;
    mmap->flags     = mmap_flags;

    int err = qnode_init(&mmap->list_head, NULL);
    if (err != 0) { 
        kfree(mmap);
        return err;
    }

    if ((err = mmap_alloc_pdbr(mmap))) {
        kfree(mmap);
        return err;
    }

    spinlock_init(&mmap->lock);

    mmap_lock(mmap);

    *pmmap = mmap;
    return 0;
}

int mmap_create(mmap_flags_t flags, mmap_t **pmmap) {
    if (pmmap == NULL || flags == 0) {
        return -EINVAL;
    }

    mmap_t *mmap;
    int err = mmap_alloc(flags, &mmap);
    if (err != 0) { return err; }

    // Start with one free region covering the address space, then reserve page zero.
    vmregion_node_t *vmregion_node;
    if ((err = vmregion_node_create_from(0, mmap->limit + 1, VmregionFree, mmap, &vmregion_node))) {
        mmap_drop(mmap);
        return err;
    }

    if ((err = mmap_insert_vmregion_node(mmap, vmregion_node))) {
        goto failure;
    }

    vmregion_node_t *null_vmregion_node;
    if ((err = vmregion_take_slice(vmregion_node, 0, PGSZ, &null_vmregion_node))) {
        goto failure;
    }

    *pmmap = mmap;
    return 0;
failure:
    vmregion_node_destroy(vmregion_node);
    mmap_drop(mmap);
    return err;
}

static void __mmap_free(mmap_t *mmap) {
    mmap_assert_locked(mmap);
    // printk("Destroying mmap.\n");

    // mmap_display(mmap);
    mmap_unlock(mmap);

    kfree(mmap);

    // printf("Destroyed mmap.\n");
}

void mmap_destroy(mmap_t *mmap) {
    mmap_assert_locked(mmap);

    // Destroy every descriptor before releasing the address-space object.
    vmregion_node_t *vmregion_node, *next_vmregion_node;
    mmap_foreach_vmregion_node_safe(mmap, vmregion_node, next_vmregion_node) {
        // int err = mmap_remove_vmregion_node(mmap, vmregion_node, &next_vmregion_node);
        // assert_eq(err, 0, "Error(%d): Failed to remove vmregion_node from mmap.\n", err);
        vmregion_node_destroy(vmregion_node);
    }

    { // rmmove the last free vmregion.
        // vmregion_get_next(vmregion_node, &next_vmregion_node);
        // vmregion_node_destroy(next_vmregion_node);
    }

    __mmap_free(mmap);
}

void mmap_drop(mmap_t *mmap) {
    bool locked = mmap_recursive_lock(mmap);
    if (__mmap_putref(mmap) <= 1) {
        mmap_destroy(mmap);
        return;
    }

    if (locked) { mmap_unlock(mmap); }
}

void mmap_increment_used_size(mmap_t *mmap, size_t size) {
    bool locked = mmap_recursive_lock(mmap);
    mmap->used_size += size;
    if (locked) { mmap_unlock(mmap); }
}

void mmap_decrement_used_size(mmap_t *mmap, size_t size) {
    bool locked = mmap_recursive_lock(mmap);
    mmap->used_size -= size;
    if (locked) mmap_unlock(mmap);
}

vmregion_node_t *mmap_get_head_vmregion_node(mmap_t *mmap) {
    mmap_assert_locked(mmap);
    return qnode_container(&mmap->list_head, vmregion_node_t, list_node);
}

int mmap_switch_to(mmap_t *mmap, uintptr_t *old_pdbr) {
    if (mmap == NULL) {
        return -EINVAL;
    }

    mmap_assert_locked(mmap);

    arch_switch_pgdir(mmap->pdbr, old_pdbr);

    return 0;
}

int mmap_copy_arglist(mmap_t *mmap, char *const __argv[], char *const __envv[], int *pargc, char *const *pargp[], char *const *penvp[]);