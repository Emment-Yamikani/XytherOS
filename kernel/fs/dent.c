#include <bits/errno.h>
#include <core/debug.h>
#include <fs/dent.h>
#include <mm/kalloc.h>
#include <string.h>


int dentry_init(Dentry *dentry) {
    int err = 0;

    if (dentry == NULL)
        return -EINVAL;

    dentry->d_count     = 1;

    dentry->d_name      = NULL;
    dentry->d_inode     = NULL;

    dentry->d_parent    = NULL;
    dentry->d_children  = NULL;

    if ((err = queue_node_init(&dentry->d_alias, dentry)))
        return err;

    if ((err = queue_node_init(&dentry->d_siblings, dentry)))
        return err;

    spinlock_init(&dentry->d_spinlock);

    return 0;
}

int dentry_alloc(const char *name, Dentry **pdp) {
    int     err = 0;
    Dentry  *dentry = NULL;

    if (name == NULL || pdp == NULL)
        return -EINVAL;

    if (NULL == (dentry = (Dentry *)kmalloc(sizeof *dentry)))
        return -ENOMEM;

    dentry_init(dentry);

    if (NULL == (dentry->d_name = strdup(name))) {
        err = -ENOMEM;
        goto error;
    }

    *pdp = dentry;

    return 0;
error:
    if (dentry) dentry_destroy(dentry);

    return err;
}

void dentry_destroy(Dentry *dentry) {
    if (dentry == NULL)
        return;

    if (dentry->d_name)
        kfree(dentry->d_name);

    if (dentry_islocked(dentry))
        dentry_unlock(dentry);
    kfree(dentry);
}

int dentry_get(Dentry *dentry) {
    dentry_assert(dentry);
    /// zero indicates reference is still valid and can be used;
    /// -EINVAL indicates no longer valid for use.
    return atomic_inc_fetch(&dentry->d_count) > 0 ? 0 : -EINVAL;
}

void denrty_put(Dentry *dentry) {
    dentry_assert(dentry);

    if (atomic_dec_fetch(&dentry->d_count) <= 0) {
        
        dentry_destroy(dentry);
    }
}