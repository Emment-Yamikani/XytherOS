#include <bits/errno.h>
#include <dev/dev.h>
#include <fs/file.h>
#include <fs/fs.h>
#include <fs/pipefs.h>
#include <mm/kalloc.h>
#include <string.h>
#include <sys/thread.h>

int     file_get(int fd, file_t **ref) {
    file_t *file = NULL;
    file_ctx_t   *fctx   = NULL; // file context

    if (ref == NULL)
        return -EINVAL;

    fctx    = current->t_fctx;
    fctx_lock(fctx);

    if ((fctx->fc_files == NULL) || (fd < 0) || (fd >= fctx->fc_nfile)) {
        fctx_unlock(fctx);
        return -EBADFD;
    }

    if ((file = fctx->fc_files[fd]) == NULL) {
        fctx_unlock(fctx);
        return -EBADFD;
    }

    flock(file);
    fctx_unlock(fctx);

    *ref = file;
    return 0;
}

int     file_free(int fd) {
    file_ctx_t   *fctx = NULL; // file context

    fctx = current->t_fctx;
    fctx_lock(fctx);

    if ((fctx->fc_files == NULL) || (fd < 0) || (fd >= fctx->fc_nfile)) {
        fctx_unlock(fctx);
        return -EBADFD;
    }

    fctx->fc_files[fd] = NULL;
    fctx_unlock(fctx);
    return 0;
}

int     file_alloc(int *ref, file_t **fref) {
    int             fd          = 0;
    int             err         = 0;
    file_t          *file       = NULL;
    file_t          **tmp       = NULL;
    file_ctx_t      *fctx   = NULL; // file context

    if (ref == NULL || fref == NULL)
        return -EINVAL;

    if ((err = falloc(&file)))
        return err;

    fctx = current->t_fctx;
    fctx_lock(fctx);

    if (fctx->fc_files == NULL) {
        if ((fctx->fc_files = kcalloc(1, sizeof (file_t *))) == NULL) {
            fctx_unlock(fctx);
            fdestroy(file);
            return -ENOMEM;
        }

        fctx->fc_nfile = 1;
    }

    for (fd = 0; fd < fctx->fc_nfile; ++fd) {
        if (fctx->fc_files[fd] == NULL) {
            fctx->fc_files[fd] = file;
            fctx_unlock(fctx);
            goto done;
        }
    }


    if ((tmp = krealloc(fctx->fc_files, ((fctx->fc_nfile + 1) * sizeof (file_t *)))) == NULL) {
        fctx_unlock(fctx);
        fdestroy(file);
        return -ENOMEM;
    }
    
    tmp[fd] = file;
    fctx->fc_files = tmp;
    fctx->fc_nfile++;

    fctx_unlock(fctx);
done:
    *ref = fd;
    *fref = file;
    return 0;
}

int     file_dup(int fd1, int fd2) {
    int err = 0;
    file_ctx_t *fctx = NULL; // file context
    file_t *file = NULL, **tmp = NULL;

    fctx = current->t_fctx;
    fctx_lock(fctx);

    if ((fctx->fc_files == NULL)) {
        fctx_unlock(fctx);
        return -EBADFD;
    }

    if (((file = fctx->fc_files[fd1]) == NULL) || (fd1 < 0) || (fd1 >= fctx->fc_nfile)) {
        fctx_unlock(fctx);
        return -EBADFD;
    }

    if (fd2 < 0)
        fd2 = fctx->fc_nfile;
    
    if (fd1 == fd2) {
        fctx_unlock(fctx);
        return fd1;
    }

    if (fd2 > 0) {
        if (fd2 >= fctx->fc_nfile) {
            if ((tmp = krealloc(fctx->fc_files, ((fd2 + 1) * sizeof (file_t *)))) == NULL) {
                fctx_unlock(fctx);
                return -ENOMEM;
            }
            memset(&tmp[fctx->fc_nfile], 0, ((fd2 + 1) - fctx->fc_nfile)*sizeof (file_t *));
            fctx->fc_files = tmp;
            fctx->fc_nfile = fd2 + 1;
        }

        if (fctx->fc_files[fd2]) {
            flock(fctx->fc_files[fd2]);
            if ((err = fclose(fctx->fc_files[fd2]))) {
                funlock(fctx->fc_files[fd2]);
                fctx_unlock(fctx);
                return err;
            }
            fctx->fc_files[fd2] = NULL;
        }
    }

    flock(file);
    if ((err = fdup(file))) {
        funlock(file);
        fctx_unlock(fctx);
        return err;
    }
    funlock(file);

    fctx->fc_files[fd2] = file;
    fctx_unlock(fctx);

    return fd2;
}

void    file_close_all(void) {
    int         file_cnt = 0;
    file_ctx_t *fctx = NULL; // file context

    fctx = current->t_fctx;
    fctx_lock(fctx);

    if (fctx->fc_files != NULL)
        file_cnt = fctx->fc_nfile;
    fctx_unlock(fctx);

    for (int fd = 0; fd < file_cnt; ++fd)
        close(fd);
}

int     file_copy(file_ctx_t *dst, file_ctx_t *src) {
    dentry_t    *cwdir      = NULL;
    dentry_t    *rootdir    = NULL;
    file_t      **files     = NULL;
    int         err         = -ENOMEM;

    if (dst == NULL || src == NULL)
        return -EINVAL;
    
    fctx_assert_locked(dst);
    fctx_assert_locked(src);
    
    if (src->fc_cwd == NULL || src->fc_root == NULL)
        return -EINVAL;

    if (src->fc_nfile != 0) {
        if (!(files = (file_t **)kmalloc(src->fc_nfile * sizeof (file_t *))))
            goto error;
    }

    dlock(src->fc_cwd);
    if (src->fc_cwd != src->fc_root)
        dlock(src->fc_root);

    if (NULL == (dst->fc_cwd  = dget(src->fc_cwd))) {
        if (src->fc_cwd != src->fc_root)
            dunlock(src->fc_root);
        dunlock(src->fc_cwd);
        return -EINVAL;
    }

    if (NULL == (dst->fc_root = dget(src->fc_root))) {
        if (src->fc_cwd != src->fc_root)
            dunlock(src->fc_root);
        dunlock(src->fc_cwd);
        return -EINVAL;
    }


    if (src->fc_cwd != src->fc_root)
        dunlock(src->fc_root);
    dunlock(src->fc_cwd);

    dst->fc_files   = files;
    dst->fc_nfile   = src->fc_nfile;

    for (int fd = 0; fd < src->fc_nfile; ++fd) {
        if (src->fc_files[fd] == NULL)
            continue;
        
        flock(src->fc_files[fd]);
        src->fc_files[fd]->f_refcnt++;
        dst->fc_files[fd] = src->fc_files[fd];
        funlock(src->fc_files[fd]);
    }

    return 0;
error:
    if (cwdir)
        kfree(cwdir);
    
    if (rootdir)
        kfree(rootdir);
    
    if (files)
        kfree(files);

    return err;
}

int     dup(int fd) {
    return file_dup(fd, -1);
}

int     dup2(int fd1, int fd2) {
    return file_dup(fd1, fd2);
}

int     sync(int fd) {
    int err = 0;
    file_t *file = NULL;
    if((err = file_get(fd, &file)))
        return err;
    err = fsync(file);
    funlock(file);
    return err;
}

int     close(int fd) {
    int err = 0;
    file_t *file = NULL;
    if((err = file_get(fd, &file)))
        return err;
    if ((err = fclose(file)))
        funlock(file);
    else
        err = file_free(fd);
    return err;    
}

int     unlink(int fd) {
    int err = 0;
    file_t *file = NULL;

    if((err = file_get(fd, &file)))
        return err;
    err = funlink(file);
    funlock(file);
    return err;    
}

int     getattr(int fd, void *attr) {
    int err = 0;
    file_t *file = NULL;

    if((err = file_get(fd, &file)))
        return err;
    err = fgetattr(file, attr);
    funlock(file);
    return err;    
}

int     setattr(int fd, void *attr) {
    int err = 0;
    file_t *file = NULL;

    if((err = file_get(fd, &file)))
        return err;
    err = fsetattr(file, attr);
    funlock(file);
    return err;    
}

int     truncate(int fd, off_t length) {
    int err = 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = ftruncate(file, length);
    funlock(file);
    return err;
}

int     fcntl(int fd, int cmd, void *argp) {
    int err = 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = ffcntl(file, cmd, argp);
    funlock(file);
    return err;
}

int     ioctl(int fd, int req, void *argp) {
    int err = 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    fioctl(file, req, argp);
    funlock(file);
    return err;
}

off_t   lseek(int fd, off_t off, int whence) {
    off_t err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = flseek(file, off, whence);
    funlock(file);
    return err;
}

ssize_t read(int fd, void *buf, size_t size) {
    ssize_t err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = fread(file, buf, size);
    funlock(file);
    return err;
}

ssize_t write(int fd, void *buf, size_t size) {
    ssize_t err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = fwrite(file, buf, size);
    funlock(file);
    return err;
}

int     create(const char *pathname, mode_t mode) {
    return open(pathname, O_WRONLY | O_CREAT | O_TRUNC, mode);
}

int     mkdirat(int fd, const char *pathname, mode_t mode) {
    int err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = fmkdirat(file, pathname, mode);
    funlock(file);
    return err;
}

int     mkdir(const char *pathname, mode_t mode) {
    return vfs_mkdir(pathname, current_cred(), mode);
}

ssize_t readdir(int fd, off_t off, void *buf, size_t count) {
    ssize_t err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = freaddir(file, off, buf, count);
    funlock(file);
    return err;
}

int     linkat(int fd, const char *oldname, const char *newname) {
    int err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = flinkat(file, oldname, newname);
    funlock(file);
    return err;
}

int     mknodat(int fd, const char *pathname, mode_t mode, int devid) {
    int err= 0;
    file_t *file = NULL;
    if ((err = file_get(fd, &file)))
        return err;
    err = fmknodat(file, pathname, mode, devid);
    funlock(file);
    return err;
}

int     mknod(const char *pathname, mode_t mode, int devid) {
    return vfs_mknod(pathname, current_cred(), mode, devid);
}

int     fstat(int fd, struct stat *buf) {
    int     err = 0;
    file_t  *file = NULL;

    if (buf == NULL)
        return -EINVAL;

    if ((err = file_get(fd, &file)))
        return err;
    
    if ((err = file_stat(file, buf))) {
        funlock(file);
        return err;
    }

    funlock(file);
    return 0;
}

int     stat(const char *restrict path, struct stat *restrict buf) {
    int         err     = 0;
    dentry_t    *dentry = NULL;
    cred_t      *cred   = NULL;

    if (path == NULL || buf == NULL)
        return -EINVAL;
    
    if ((err = vfs_lookup(path, cred, O_EXCL, &dentry)))
        return err;
    
    if (dentry->d_inode == NULL) {
        dclose(dentry);
        return -EBADF;
    }

    ilock(dentry->d_inode);
    if ((err = istat(dentry->d_inode, buf))) {
        iunlock(dentry->d_inode);
        dclose(dentry);
        return -EBADF;
    }
    iunlock(dentry->d_inode);

    dclose(dentry);
    return 0;
}

int     lstat(const char *restrict path, struct stat *restrict buf) {
    return stat(path, buf);
}

int     fstatat(int fd, const char *restrict path, struct stat *restrict buf, int flag) {
    int         err         = 0;
    file_t      *dir_file   = NULL;
    dentry_t    *dentry     = NULL;
    cred_t      *cred       = NULL;
    (void)      flag;

    if ((err = file_get(fd, &dir_file)))
        return err;
    
    if (dir_file->f_dentry == NULL) {
        funlock(dir_file);
        return err;
    }

    dlock(dir_file->f_dentry);
    if ((err = vfs_lookupat(path, dir_file->f_dentry, cred, O_EXCL, &dentry))) {
        dunlock(dir_file->f_dentry);
        funlock(dir_file);
        return err;
    }
    dunlock(dir_file->f_dentry);
    funlock(dir_file);

    if (dentry->d_inode == NULL) {
        dclose(dentry);
        return -EBADF;
    }

    ilock(dentry->d_inode);
    if ((err = istat(dentry->d_inode, buf))) {
        iunlock(dentry->d_inode);
        dclose(dentry);
        return -EBADF;
    }
    iunlock(dentry->d_inode);

    dclose(dentry);
    return 0;
}

int     chown(const char *path, uid_t owner, gid_t group) {
    int         err     = 0;
    dentry_t    *dentry = NULL;
    cred_t      *cred   = NULL;

    if (path == NULL)
        return -EINVAL;
    
    if ((err = vfs_lookup(path, cred, O_EXCL, &dentry)))
        return err;
    
    if (dentry->d_inode == NULL) {
        dclose(dentry);
        return -EBADF;
    }

    ilock(dentry->d_inode);
    if ((err = ichown(dentry->d_inode, owner, group))) {
        iunlock(dentry->d_inode);
        dclose(dentry);
        return -EBADF;
    }
    iunlock(dentry->d_inode);

    dclose(dentry);
    return 0;
}

int     fchown(int fd, uid_t owner, gid_t group) {
    int     err     = 0;
    file_t  *file   = NULL;

    if ((err = file_get(fd, &file)))
        return err;

    err = file_chown(file, owner, group);
    funlock(file);

    return err;
}

int     pipe(int fds[2]) {
    int      err     = 0;
    pipe_t   *pipe   = NULL;
    int      fildes0 = 0, fildes1 = 0;
    dentry_t *d0     = NULL, *d1  = NULL;
    file_t   *fd0    = NULL, *fd1 = NULL;

    if (fds == NULL)
        return -EINVAL;

    if ((err = pipe_mkpipe(&pipe)))
        return err;

    if ((err = file_alloc(&fildes0, &fd0)))
        goto error;

    if ((err = file_alloc(&fildes1, &fd1)))
        goto error;

    if ((err = dalloc("dentry_pipe-r", &d0)))
        goto error;

    if ((err = dalloc("dentry-pipe-w", &d1)))
        goto error;

    if ((err = iadd_alias(pipe->p_iread, d0)))
        goto error;
    
    if ((err = iadd_alias(pipe->p_iwrite, d1)))
        goto error;

    iunlock(pipe->p_iread);
    dunlock(d0);
    
    iunlock(pipe->p_iwrite);
    dunlock(d1);

    fd0->f_off      = 0;
    fd0->f_dentry   = d0;
    fd0->f_oflags   |= O_RDONLY;
    fd0->f_oflags   &= ~(O_CLOEXEC | O_NONBLOCK);
    funlock(fd0);


    fd1->f_off      = 0;
    fd1->f_dentry   = d1;
    fd1->f_oflags   |= O_WRONLY;
    fd1->f_oflags   &= ~(O_CLOEXEC | O_NONBLOCK);
    funlock(fd1);

    pipe_unlock(pipe);

    fds[0] = fildes0;
    fds[1] = fildes1;
    return 0;
error:
    if (d0) dclose(d0);
    if (d1) dclose(d1);

    // TODO: release the pipe descriptor.
    printk("Failed to create a pipe, error: %d\n");
    return err;
}