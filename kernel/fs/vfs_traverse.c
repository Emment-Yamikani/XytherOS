#include <bits/errno.h>
#include <fs/dentry.h>
#include <fs/fs.h>
#include <fs/inode.h>
#include <string.h>
#include <sys/thread.h>

/**
 * @brief 
 * 
 * @param path 
 * @param cred 
 * @param oflags 
 * @return int 
 * 
 * @ERROR:
 * -ENINVAL: An invalid paremeter was passed.
 * -ENOTDIR: Traversal attempted on a non direcory. Or one of the components
 * of the path isn't, in fact, a directory.
 * -ECCESS: You do not have sufficient permissions to walk the path.
 * -ENOENT: An entry in the path was not found.
 */
int vfs_traverse_path(vfspath_t *path, cred_t *cred, int oflags) {
    int         err      = 0;
    inode_t     *ip      = NULL;
    dentry_t    *dentry  = NULL;

    if (path == NULL)
        return -EINVAL;
    
    /**
     * If no directory was specified
     * use the system's root directory.
     * TODO: perhaps use the thread's group
     * root directory?*/
    if (path->directory == NULL) {
        if (current_fctx()) {
            fctx_lock(current_fctx());
            if (path->directory != current_fctx()->fc_cwd) {
                if (path->directory)
                    dclose(path->directory);
                if (vfspath_isabsolute(path))
                    path->directory = current_fctx()->fc_root;
                else
                    path->directory = current_fctx()->fc_cwd;
                dlock(path->directory);
                dopen(path->directory);
            }
            fctx_unlock(current_fctx());
        } 
        
        if (path->directory == NULL) {
            if ((path->directory = vfs_getdroot()) == NULL) {
                return -ENOTDIR;
            }
        }
    }

    /**
     * If the root directory was intended
     * do not perform the traversal but rather
     * use the currently set directory as the
     * requested path entry.*/
    if (string_eq("/", path->absolute)) {
        if (current_fctx()) {
            fctx_lock(current_fctx());
            if (path->directory != current_fctx()->fc_root) {
                if (path->directory)
                    dclose(path->directory);
                path->directory = current_fctx()->fc_root;
                dlock(path->directory);
                dopen(path->directory);
            }
            fctx_unlock(current_fctx());
        }

        // test if we have an inode, needed especially during kernel startup.
        if (path->directory->d_inode) {
            ilock(path->directory->d_inode);
            if ((err = icheck_perm(path->directory->d_inode, cred, oflags))) {
                iunlock(path->directory->d_inode);
                return err;
            }
            iunlock(path->directory->d_inode);
        }

        path->dentry    = path->directory;
        path->directory = NULL;
        return 0;
    }

    foreach(token, &path->tokenized[path->tok_index]) {
        dentry          = NULL;
        path->dentry    = NULL;
        path->token     = token;

        ilock(path->directory->d_inode);
        if ((err = icheck_perm(path->directory->d_inode, cred, oflags))) {
            iunlock(path->directory->d_inode);
            // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n",
                // __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
            goto error;
        }

        // Ensure that this path->directory is actually refering to a directory.
        if ((err = IISDIR(path->directory->d_inode) ? 0 : -ENOTDIR)) {
            // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n",
                // __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
            iunlock(path->directory->d_inode);
            goto error;
        }
        iunlock(path->directory->d_inode);

        /** Lookup 'token' in the dentry cache
         * of children. unlock 'path->directory' if err != -ENOENT*/
        if ((err = dlookup(path->directory, token, &dentry))) {
            if (err == -ENOENT) { // path component not found.
                // if current path->directory has an inode follow the fs-specific lookup.
                if (path->directory->d_inode != NULL) {
                    foreach(token, &path->tokenized[path->tok_index]) {
                        dentry          = NULL;
                        path->dentry    = NULL;
                        path->token     = token;

                        ilock(path->directory->d_inode);
                        if ((err = ilookup(path->directory->d_inode, token, &ip))) {
                            // reached the end of the path?
                            if (string_eq(token, path->lasttoken)) {
                                vfspath_set_lasttoken(path);
                            }
                            iunlock(path->directory->d_inode);
                            // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n",
                                // __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
                            goto error;
                        }
                        iunlock(path->directory->d_inode);

                        // found the inode.
                        if ((err = icheck_perm(ip, cred, oflags))) {
                            irelease(ip);
                            // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n",
                                // __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
                            goto error;
                        }

                        // add dentry alias to the new inode.
                        if ((err = imkalias(ip, token, &dentry))) {
                            irelease(ip);
                            // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n",
                                // __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
                            goto error;
                        }

                        // bind the new dentry to the directory tree.
                        if ((err = dbind(path->directory, dentry))) {
                            irelease(ip);
                            // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n",
                                // __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
                            goto error;
                        }

                        err = IISDIR(ip) ? 0: -ENOTDIR;
                        irelease(ip);

                        // reached the end of the path?
                        if (string_eq(token, path->lasttoken)) {
                            vfspath_set_lasttoken(path);
                            /**
                             * we've reached the end of the path.
                             * path fully traversed but does not refer to a directory.
                             * user specified '/' at the end of the pathname,
                             * therefore this is an error.*/
                            if (vfspath_isdir(path) && err == -ENOTDIR) {
                                dclose(dentry);
                                goto error;
                            }
                            path->dentry = dentry;
                        } else { // we've not reached the end of the path.
                            if (err == - ENOTDIR) {
                                /** ERROR: the next token cannot be traversed
                                 * because it is not a directory.*/
                                // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n",
                                    // __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
                                dclose(dentry);
                                goto error;
                            }

                            dclose(path->directory);
                            // next token is a dir and can be followed.
                            path->tok_index++;
                            path->directory     = dentry;
                        }
                    }
                    goto done;
                }
                panic("%s:%d: Direcetory has no inode to follow, error: %d\n", __FILE__, __LINE__, err);
            }
            goto error;
        }

        /** If dentry has an inode
         * check if we have the neccessary permission
         * to access the inode.*/
        if (dentry->d_inode) {
            ilock(dentry->d_inode);
            if ((err = icheck_perm(dentry->d_inode, cred, oflags))) {
                iunlock(dentry->d_inode);
                dclose(dentry);
                // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n",
                    // __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
                goto error;
            }
            err = IISDIR(dentry->d_inode) ? 0: -ENOTDIR;
            iunlock(dentry->d_inode);
        }

        // reached the end of the path?
        if (string_eq(token, path->lasttoken)) {
            vfspath_set_lasttoken(path);
            /**
             * we've reached the end of the path.
             * path fully traversed but does not refer to a directory.
             * user specified '/' at the end of the pathname,
             * therefore this is an error.*/
            if (vfspath_isdir(path) && err == -ENOTDIR) {
                dclose(dentry);
                goto error;
            }
            path->dentry = dentry;
        } else { // we've not reached the end of the path.
            if (err == -ENOTDIR) {
                /** ERROR: the next token cannot be traversed
                 * because it is not a directory.*/
                // printk("%s:%d: %s() failed to lookup: '%s/%s', index: %d err: %d\n",
                    // __FILE__, __LINE__, __func__, path->directory->d_name, path->token, path->tok_index, err);
                dclose(dentry);
                goto error;
            }

            dclose(path->directory);
            // next token is a dir and can be followed.
            path->tok_index++;
            path->directory     = dentry;
        }
    }

done:
    /*close the directory, we do not need it anymore
    not have access to the directory, one only need
    access to path->dentry->d_parent.*/
    dclose(path->directory);
    path->directory = NULL;

    return 0;
error:
    return err;
}

int vfs_resolve_path(const char *pathname, dentry_t *dir,  cred_t *cred, int oflags, vfspath_t **rp) {
    int       err   = 0;
    vfspath_t *path = NULL;
    inode_t   *newip= NULL;
    inode_t   *oldip= NULL;
    
    if (pathname == NULL || rp == NULL)
        return -EINVAL;

    if ((err = vfspath_parse(pathname, 0, &path)))
        return err;

    path->directory = dir;
    
    if ((err = vfs_traverse_path(path, cred, oflags)) == 0) {
        /** Open the requested inode.
         * to iopen() 2nd parameter will overwrite path->dentry->d_inode,
         * this will happen in the even that the call to iopen causes another inode to be
         * allocated, is in the case of device multiplexing, e.g opening '/dev/ptmx'.
         * This will ensure that opening a device multiplexer will result in
         * creation of a unique device file descriptor.
         * TODO: assess the effectiveness of this design decision.*/
        dopen(path->dentry);
        dunlock(path->dentry);
        oldip = path->dentry->d_inode;
        if (oldip != NULL) {
            ilock(oldip);
            if ((err = iopen(oldip, &newip))) {
                iunlock(oldip);
                dlock(path->dentry);
                dclose(path->dentry);
                *rp = path;
                return err;
            }

            /** We prefer the new inode over the old one,
             * So close the old inode struct.*/
            if (newip != NULL) {
                newip->i_sb  = oldip->i_sb;
                newip->i_ino = oldip->i_ino;
                newip->i_ops = oldip->i_ops;
                path->dentry->d_inode = newip;
                iunlock(newip);
                irelease(oldip);
            } else iunlock(oldip);
        }
        dlock(path->dentry);
        dclose(path->dentry);
    }

    *rp = path;
    return err;
}

int vfs_lookupat(const char *pathname, dentry_t *dir, cred_t *cred, int oflags, dentry_t **pdp) {
    int         err     = 0;
    vfspath_t   *path   = NULL;

    if ((err = vfs_resolve_path(pathname, dir, cred, oflags, &path))) {
        if (path) {
            assert(path->directory, "On error, path has no directory\n");
            dclose(path->directory);
        }
        goto error;
    }

    assert(path->directory == NULL, "On success, path has directory\n");
    assert(path->dentry, "On success, path has no dentry\n");

    if (pdp)
        *pdp = path->dentry;
    else
        dclose(path->dentry);

    path_free(path);
    return 0;
error:
    if (path)
        path_free(path);
    return err;
}

int vfs_lookup(const char *pathname, cred_t *__cred, int oflags, dentry_t **pdp) {
    return vfs_lookupat(pathname, NULL, __cred, oflags, pdp);
}