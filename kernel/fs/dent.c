#include <bits/errno.h>
#include <core/debug.h>
#include <fs/dent.h>
#include <mm/kalloc.h>
#include <string.h>

static void dentry_destroy(Dentry *dentry) {
    if (dentry == NULL)
        return;

    if (dentry->d_name)
        kfree(dentry->d_name);

    dentry_recursive_lock(dentry);

    dentry_unbind(dentry);

    dentry_unlock(dentry);
    kfree(dentry);
}

int dentry_init(const char *name, Dentry *dentry) {
    if (dentry == NULL)
        return -EINVAL;

    dentry->d_count     = 1;

    dentry->d_name      = NULL;
    dentry->d_inode     = NULL;

    dentry->d_parent    = NULL;

    INIT_LIST_HEAD(&dentry->d_alias);
    INIT_LIST_HEAD(&dentry->d_children);
    INIT_LIST_HEAD(&dentry->d_siblings);

    spinlock_init(&dentry->d_children_lock);
    spinlock_init(&dentry->d_spinlock);

    if (NULL == (dentry->d_name = strdup(name))) {
        return -ENOMEM;
    }
    return 0;
}

int dentry_alloc(const char *name, Dentry **pdp) {
    int     err = 0;
    Dentry  *dentry = NULL;

    if (name == NULL || pdp == NULL)
        return -EINVAL;

    if (NULL == (dentry = (Dentry *)kmalloc(sizeof *dentry)))
        return -ENOMEM;

    if ((err = dentry_init(name, dentry)))
        goto error;

    dentry_lock(dentry);
    *pdp = dentry;

    return 0;
error:
    if (dentry) dentry_destroy(dentry);

    return err;
}

int dentry_open(Dentry *dentry) {
    dentry_assert(dentry);
    /// zero indicates reference is still valid and can be used;
    /// -EINVAL indicates no longer valid for use.
    return atomic_inc_fetch(&dentry->d_count) > 0 ? 0 : -EINVAL;
}

void dentry_close(Dentry *dentry) {
    dentry_assert(dentry);

    if (atomic_dec_fetch(&dentry->d_count) <= 0) {
        dentry_destroy(dentry);
    } else {
        if (dentry_islocked(dentry))
            dentry_unlock(dentry);
    }
}

int dentry_bind(Dentry *dir, Dentry *child) {
    if (dir == NULL || child == NULL)
        return -EINVAL;

    dentry_assert_locked(child);

    spin_lock(&dir->d_children_lock);

    list_add(&child->d_siblings, &dir->d_children);

    spin_unlock(&dir->d_children_lock);

    child->d_parent = dir;

    atomic_inc(&child->d_count);
    atomic_inc(&dir->d_count);

    return 0;
}

int dentry_unbind(Dentry *dentry) {
    if (dentry == NULL || dentry->d_parent == NULL)
        return -EINVAL;

    dentry_assert_locked(dentry);

    spin_lock(&dentry->d_parent->d_children_lock);

    list_remove(&dentry->d_siblings);

    spin_unlock(&dentry->d_parent->d_children_lock);

    atomic_dec(&dentry->d_parent->d_count);
    atomic_dec(&dentry->d_count);

    dentry->d_parent = NULL;
    return 0;
}

int dentry_lookup(Dentry *dir, const char *name, Dentry **pdp) {
    Dentry *dentry, *next;

    if (dir == NULL || name == NULL)
        return -EINVAL;

    spin_lock(&dir->d_children_lock);

    list_foreach_entry_safe(dentry, next, &dir->d_children, d_siblings) {
        dentry_lock(dentry);
        if (string_eq(dentry->d_name, name)) {
            if (pdp) {
                dentry_open(dentry);
                *pdp = dentry;
            } else dentry_unlock(dentry);

            spin_unlock(&dir->d_children_lock);
            return 0;
        }
        dentry_unlock(dentry);
    }

    spin_unlock(&dir->d_children_lock);
    return -ENOENT;
}