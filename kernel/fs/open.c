#include <bits/errno.h>
#include <dev/dev.h>
#include <fs/file.h>
#include <fs/fs.h>
#include <fs/pipefs.h>
#include <mm/kalloc.h>
#include <string.h>
#include <sys/thread.h>

int     vfs_openat(dentry_t *dir, const char *pathname, int oflags, mode_t mode, dentry_t **pdp) {
    int         err     = 0;
    vfspath_t   *path   = NULL;

    // openat() must only be called by threads.
    current_assert();
    if (dir)
        dassert_locked(dir);

    if ((err = vfs_resolve_path(pathname, dir, current_cred(), oflags, &path))) {
        if ((err == -ENOENT) && (oflags & O_CREAT)) {
            if (vfspath_isdir(path) || (oflags & O_DIRECTORY)) {
                err = -EINVAL;
                dclose(path->directory);
                goto error;
            }

            // Ensure to only create the file if path->token is the last component in the path.
            if (vfspath_islasttoken(path)) {
                ilock(path->directory->d_inode);
                if ((err = icreate(path->directory->d_inode, path->token, mode & ~S_IFMT))) {
                    iunlock(path->directory->d_inode);
                    dclose(path->directory);
                    goto error;
                }
                iunlock(path->directory->d_inode);
                /**
                 * NOTE: no need of dclose(path->directory);
                 * followed by path_free(path); here
                 * because vfs_traverse_path() below can operate on a 'path'
                 * starting at the last traversed token, thus improving performance.
                 **/
                // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n", __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
                if ((err = vfs_traverse_path(path, current_cred(), oflags))) {
                    dclose(path->dentry);
                    goto error;
                }
                goto found;
            }

            // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n", __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
            // remaining token is not the lat component of path.
            dclose(path->directory);
            goto error;
        }
        
        if (path) {
            assert(path->directory, "On error, path has no directory\n");
            dclose(path->directory);
        }
        goto error;
    }
    
found:
    assert(path->directory == NULL, "On success, path has directory\n");
    assert(path->dentry, "On success, path has no dentry\n");

    *pdp = path->dentry;

    path_free(path);
    return 0;
error:
    if (path)
        path_free(path);
    return err;
}

int     open(const char *pathname, int oflags, mode_t mode) {
    int         fd      = 0;
    int         err     = 0;
    vfspath_t   *path   = NULL;
    file_t      *file   = NULL;
    dentry_t    *dentry = NULL;

    if (pathname == NULL)
        return -EINVAL;

    printk("%s:%d: %s(%s, %o, %o);\n",
        __FILE__, __LINE__, __func__, pathname, oflags, mode);

    // open() must only be called by threads.
    current_assert();

    if ((err = vfs_resolve_path(pathname, dentry, current_cred(), oflags, &path))) {
        if ((err == -ENOENT) && (oflags & O_CREAT)) {
            if (vfspath_isdir(path) || (oflags & O_DIRECTORY)) {
                err = -EINVAL;
                dclose(path->directory);
                goto error;
            }

            // Ensure to only create the file if path->token is the last component in the path.
            if (vfspath_islasttoken(path)) {
                ilock(path->directory->d_inode);
                if ((err = icreate(path->directory->d_inode, path->token, mode & ~S_IFMT))) {
                    iunlock(path->directory->d_inode);
                    dclose(path->directory);
                    goto error;
                }
                iunlock(path->directory->d_inode);
                /**
                 * NOTE: no need of dclose(path->directory);
                 * followed by path_free(path); here
                 * because vfs_traverse_path() below can operate on a 'path'
                 * starting at the last traversed token, thus improving performance.
                 **/
                // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n", __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
                if ((err = vfs_traverse_path(path, current_cred(), oflags))) {
                    dclose(path->dentry);
                    goto error;
                }
                goto found;
            }

            // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n", __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
            // remaining token is not the lat component of path.
            dclose(path->directory);
            goto error;
        }
        
        if (path) {
            assert(path->directory, "On error, path has no directory. err: %d\n", err);
            dclose(path->directory);
        } else {
            printk("%s:%d: [Warning...]: is unlocking dentry here safe?: Err: %d\n", __FILE__, __LINE__, err);
            dunlock(dentry);
        }
        goto error;
    }
    
found:
    assert(path->directory == NULL, "On success, path has directory\n");
    assert(path->dentry, "On success, path has no dentry\n");

    if ((err = file_alloc(&fd, &file))) {
        dclose(path->dentry);
        goto error;
    }

    file->fops      = NULL;
    file->f_dentry  = path->dentry;
    file->f_oflags  = oflags;
    
    funlock(file);
    dunlock(path->dentry);

    path_free(path);
    return fd;
error:
    if (path)
        path_free(path);
    return err;
}

int     openat(int fd, const char *pathname, int oflags, mode_t mode) {
    int         err     = 0;
    vfspath_t   *path   = NULL;
    file_t      *file   = NULL;
    dentry_t    *dentry = NULL;

    // openat() must only be called by threads.
    current_assert();

    if ((err = file_get(fd, &file)))
        return err;
    
    dlock(file->f_dentry);
    if ((err = dopen(file->f_dentry))) {
        dunlock(file->f_dentry);
        funlock(file);
        return err;
    }

    dentry = file->f_dentry;
    funlock(file);

    if ((err = vfs_resolve_path(pathname, dentry, current_cred(), oflags, &path))) {
        if ((err == -ENOENT) && (oflags & O_CREAT)) {
            if (vfspath_isdir(path) || (oflags & O_DIRECTORY)) {
                err = -EINVAL;
                dclose(path->directory);
                goto error;
            }

            // Ensure to only create the file if path->token is the last component in the path.
            if (vfspath_islasttoken(path)) {
                ilock(path->directory->d_inode);
                if ((err = icreate(path->directory->d_inode, path->token, mode & ~S_IFMT))) {
                    iunlock(path->directory->d_inode);
                    dclose(path->directory);
                    goto error;
                }
                iunlock(path->directory->d_inode);
                /**
                 * NOTE: no need of dclose(path->directory);
                 * followed by path_free(path); here
                 * because vfs_traverse_path() below can operate on a 'path'
                 * starting at the last traversed token, thus improving performance.
                 **/
                // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n", __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
                if ((err = vfs_traverse_path(path, current_cred(), oflags))) {
                    dclose(path->dentry);
                    goto error;
                }
                goto found;
            }

            // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n", __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
            // remaining token is not the lat component of path.
            dclose(path->directory);
            goto error;
        }
        
        if (path) {
            assert(path->directory, "On error, path has no directory\n");
            dclose(path->directory);
        } else {
            printk("%s:%d: [Warning...]: is unlocking dentry here safe?\n", __FILE__, __LINE__);
            dunlock(dentry);
        }
        goto error;
    }
    
found:
    assert(path->directory == NULL, "On success, path has directory\n");
    assert(path->dentry, "On success, path has no dentry\n");

    if ((err = file_alloc(&fd, &file))) {
        dclose(path->dentry);
        goto error;
    }

    file->fops      = NULL;
    file->f_dentry  = path->dentry;
    file->f_oflags  = oflags;
    
    funlock(file);
    dunlock(path->dentry);

    path_free(path);
    return fd;
error:
    if (path)
        path_free(path);
    return err;
}