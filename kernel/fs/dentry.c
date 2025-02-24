#include <bits/errno.h>
#include <core/debug.h>
#include <fs/dentry.h>
#include <mm/kalloc.h>
#include <string.h>

static void ddestroy(dentry_t *dentry) {
    if (dentry == NULL)
        return;

    if (dentry->d_name)
        kfree(dentry->d_name);

    drecursive_lock(dentry);

    dunbind(dentry);

    dunlock(dentry);
    kfree(dentry);
}

int dinit(const char *name, dentry_t *dentry) {
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

int dalloc(const char *name, dentry_t **pdp) {
    int     err = 0;
    dentry_t  *dentry = NULL;

    if (name == NULL || pdp == NULL)
        return -EINVAL;

    if (NULL == (dentry = (dentry_t *)kmalloc(sizeof *dentry)))
        return -ENOMEM;

    if ((err = dinit(name, dentry)))
        goto error;

    dlock(dentry);
    *pdp = dentry;

    return 0;
error:
    if (dentry) ddestroy(dentry);

    return err;
}

int dopen(dentry_t *dentry) {
    if (dentry == NULL)
        return -EINVAL;

    dassert_locked(dentry);
    /// zero indicates reference is still valid and can be used;
    /// -EINVAL indicates no longer valid for use.
    return atomic_inc_fetch(&dentry->d_count) > 0 ? 0 : -EINVAL;
}

dentry_t *dget(dentry_t *dentry) {
    return !dopen(dentry) ? dentry : NULL;
}

void dclose(dentry_t *dentry) {
    dassert_locked(dentry);

    if (atomic_dec_fetch(&dentry->d_count) <= 0) {
        ddestroy(dentry);
    } else {
        if (dislocked(dentry))
            dunlock(dentry);
    }
}

int dbind(dentry_t *dir, dentry_t *child) {
    if (dir == NULL || child == NULL)
        return -EINVAL;

    dassert_locked(child);

    spin_lock(&dir->d_children_lock);

    list_add(&child->d_siblings, &dir->d_children);

    spin_unlock(&dir->d_children_lock);

    child->d_parent = dir;

    atomic_inc(&child->d_count);
    atomic_inc(&dir->d_count);

    return 0;
}

int dunbind(dentry_t *dentry) {
    if (dentry == NULL || dentry->d_parent == NULL)
        return -EINVAL;

    dassert_locked(dentry);

    spin_lock(&dentry->d_parent->d_children_lock);

    list_remove(&dentry->d_siblings);

    spin_unlock(&dentry->d_parent->d_children_lock);

    atomic_dec(&dentry->d_parent->d_count);
    atomic_dec(&dentry->d_count);

    dentry->d_parent = NULL;
    return 0;
}

int dlookup(dentry_t *dir, const char *name, dentry_t **pdp) {
    dentry_t *dentry, *next;

    if (dir == NULL || name == NULL)
        return -EINVAL;

    spin_lock(&dir->d_children_lock);

    list_foreach_entry_safe(dentry, next, &dir->d_children, d_siblings) {
        dlock(dentry);
        if (string_eq(dentry->d_name, name)) {
            if (pdp) {
                dopen(dentry);
                *pdp = dentry;
            } else dunlock(dentry);

            spin_unlock(&dir->d_children_lock);
            return 0;
        }
        dunlock(dentry);
    }

    spin_unlock(&dir->d_children_lock);
    return -ENOENT;
}