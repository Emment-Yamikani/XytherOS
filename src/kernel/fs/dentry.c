#include <bits/errno.h>
#include <core/debug.h>
#include <fs/dentry.h>
#include <mm/kalloc.h>
#include <string.h>
#include <xytherOs_string.h>

static void ddestroy(dentry_t *dentry) {
    if (dentry == NULL) {
        return;
    }

    if (dentry->d_name)
        kfree(dentry->d_name);

    drecursive_lock(dentry);

    dunbind(dentry);

    dunlock(dentry);
    kfree(dentry);
}

int dinit(const char *name, dentry_t *dentry) {
    if (dentry == NULL) {
        return -EINVAL;
    }

    dentry->d_count = 1;

    INIT_LIST_HEAD(&dentry->d_alias);
    INIT_LIST_HEAD(&dentry->d_children);
    INIT_LIST_HEAD(&dentry->d_siblings);

    spinlock_init(&dentry->d_spinlock);

    if (NULL == (dentry->d_name = strdup(name))) {
        return -ENOMEM;
    }

    return 0;
}

int dalloc(const char *name, dentry_t **pdp) {
    int         err     = 0;
    dentry_t    *dentry = NULL;

    if (name == NULL || pdp == NULL) {
        return -EINVAL;
    }

    if (NULL == (dentry = (dentry_t *)kzalloc(sizeof *dentry))) {
        return -ENOMEM;
    }

    if ((err = dinit(name, dentry))) {
        goto error;
    }

    dlock(dentry);

    *pdp = dentry;

    return 0;
error:
    if (dentry) ddestroy(dentry);

    return err;
}

int dopen(dentry_t *dentry) {
    if (dentry == NULL) {
        return -EINVAL;
    }

    dassert_locked(dentry);
    /// zero indicates reference is still valid and can be used;
    /// -EINVAL indicates no longer valid for use.
    return atomic_inc_fetch(&dentry->d_count) > 0 ? 0 : -EINVAL;
}

dentry_t *dget(dentry_t *dentry) {
    return !dopen(dentry) ? dentry : NULL;
}

void dput(dentry_t *dentry) {
    if (dentry == NULL) {
        return;
    }

    if (atomic_fetch_dec(&dentry->d_count) == 1) {
        dclose(dentry);
    }
}

void dclose(dentry_t *dentry) {
    dassert_locked(dentry);

    if (atomic_dec_fetch(&dentry->d_count) <= 0) {
        ddestroy(dentry);
    } else {
        if (dislocked(dentry)) {
            dunlock(dentry);
        }
    }
}

int dbind(dentry_t *dir, dentry_t *child) {
    if (dir == NULL || child == NULL) {
        return -EINVAL;
    }

    dassert_locked(child);

    list_add(&child->d_siblings, &dir->d_children);

    child->d_parent = dir;

    atomic_inc(&child->d_count);
    atomic_inc(&dir->d_count);

    return 0;
}

int dunbind(dentry_t *dentry) {
    if (dentry == NULL || dentry->d_parent == NULL) {
        return -EINVAL;
    }

    dassert_locked(dentry);

    list_remove(&dentry->d_siblings);

    atomic_dec(&dentry->d_parent->d_count);
    atomic_dec(&dentry->d_count);

    dentry->d_parent = NULL;
    return 0;
}

int dlookup(dentry_t *dir, const char *name, dentry_t **pdp) {
    dentry_t *dentry, *next;

    if (dir == NULL || name == NULL) {
        return -EINVAL;
    }

    list_foreach_entry_safe(dentry, next, &dir->d_children, d_siblings) {
        dlock(dentry);
        if (string_eq(dentry->d_name, name)) {
            if (pdp) {
                dopen(dentry);
                *pdp = dentry;
            } else {
                dunlock(dentry);
            }
            return 0;
        }
        dunlock(dentry);
    }

    return -ENOENT;
}

int dretrieve_path(dentry_t *dentry, char **ret, size_t *rlen) {
    int         err     = 0;
    size_t      nt      = 1;  // No. of tokens.
    dentry_t    *next   = NULL;
    char        **tokens= NULL;
    char        *path   = NULL;


    if (dentry == NULL || dentry->d_name == NULL || ret == NULL)
        return -EINVAL;
    
    if (NULL == (tokens = (char **)kcalloc(2, sizeof (char *))))
        return -ENOMEM;

    if (NULL == (tokens[nt - 1] = strdup(dentry->d_name))) {
        kfree(tokens);
        return -ENOMEM;
    }

    forlinked(parent, dentry->d_parent, next) {
        char **tmp = NULL; // tmp tokenized path.
        nt  += 1; // we have a valid parent, so count it.

        if (NULL == (tmp = (char **)krealloc(tokens, (nt + 1) * sizeof (char *)))) {
            tokens_free(tokens);
            return -ENOMEM;
        }

        tokens = tmp;
        if (nt == 1) { // First time allocating tokens, so clear it.
            memset(tokens, 0, (nt + 1) * sizeof (char *));
        }

        dlock(parent);
        next = parent->d_parent;
        if (NULL == (tokens[nt - 1] = strdup(parent->d_name))) {
            dunlock(parent);
            tokens_free(tokens);
            return -ENOMEM;
        }
        dunlock(parent);

        tokens[nt] = NULL;
    }

    // allocate space for the resultant absolute path.
    if (NULL == (path = (char *)kmalloc(2)))
        goto error;

    *path   = '/';  // account for the root fs.
    path[1] = '\0'; // terminate the string.

    if (rlen) *rlen = 1;    // if requested return the path length.

    foreach_reverse(token, &tokens[nt - 1]) {
        static size_t   len     = 1;    // keep track of the length of the resultant absolute path.
        size_t          toklen  = 0;    // length of the token.
        static size_t   off     = 1;    // where to write the next token.
        char            *tmp    = NULL; // for temporal use.

        if (string_eq(token, "/")){
            nt -= 1; // we discard '/'
            continue; // skip the root fs "/".
        }

        /// get the length of the current token. 
        /// NOTE: 'nt > 1 ? 1 : 0' this is to account for separator(/) and the path terminator(\0).
        /// if nt > 1, more tokens ahead so leave room for '/'.
        len += (toklen = strlen(token)) + (nt > 1 ? 1 : 0);

        // reallocate space for more tokens.
        if (NULL == (tmp = (char *)krealloc(path, len + 1))) {
            err = -ENOMEM;
            goto error;
        }

        path = tmp;
        strncpy(path + off, token, toklen); // copy the current token.
        off += toklen; // increment the offset for the next token if any.

        nt -= 1; // decrement No. of tokens.
        if (nt != 0) {
            path[len - 1] = '/'; // not the last token add a '/'.
            off += 1;
        } else path[len] = '\0'; // we're at the end, terminate the path.

        if (rlen) *rlen = len;  // if requested return the path length.
    }

    tokens_free(tokens);

    *ret = path;

    return 0;
error:
    if (tokens != NULL)
        tokens_free(tokens);

    if (path != NULL)
        kfree(path);

    return err;
}