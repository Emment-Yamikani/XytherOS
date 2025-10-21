#pragma once
#include <core/types.h>
#include <fs/dentry.h>
#include <fs/stat.h>
#include <fs/fcntl.h>
#include <fs/inode.h>
#include <fs/cred.h>
#include <mm/mmap.h>
#include <sync/spinlock.h>
#include <sys/_utsname.h>

typedef struct file_t file_t;

typedef struct fops_t {
    int     (*feof)(file_t *file);
    int     (*fsync)(file_t *file);
    int     (*fclose)(file_t *file);
    int     (*funlink)(file_t *file);
    int     (*fgetattr)(file_t *file, void *attr);
    int     (*fsetattr)(file_t *file, void *attr);
    int     (*ftruncate)(file_t *file, off_t length);
    int     (*ffcntl)(file_t *file, int cmd, void *argp);
    int     (*fioctl)(file_t *file, int req, void *argp);
    off_t   (*flseek)(file_t *file, off_t off, int whence);
    ssize_t (*fread)(file_t *file, void *buf, size_t size);
    ssize_t (*fwrite)(file_t *file, const void *buf, size_t size);
    int     (*fcreate)(file_t *dir, const char *filename, mode_t mode);
    int     (*fmkdirat)(file_t *dir, const char *filename, mode_t mode);
    ssize_t (*freaddir)(file_t *dir, off_t off, void *buf, size_t count);
    int     (*flinkat)(file_t *dir, const char *oldname, const char *newname);
    int     (*fmknodat)(file_t *dir, const char *filename, mode_t mode, int devid);
    int     (*fmmap)(file_t *file, vmr_t *region);
    int     (*fstat)(file_t *file, struct stat *buf);
    int     (*fchown)(file_t *file, uid_t owner, gid_t group);
} fops_t;

typedef struct file_t {
    off_t       f_off;
    int         f_oflags;
    long        f_refcnt;
    dentry_t    *f_dentry;
    fops_t      *fops;
    spinlock_t  f_lock;
} file_t;

#define fassert(file)         ({ assert(file, "No file pointer."); })
#define flock(file)           ({ fassert(file); spin_lock(&(file)->f_lock); })
#define funlock(file)         ({ fassert(file); spin_unlock(&(file)->f_lock); })
#define fislocked(file)       ({ fassert(file); spin_islocked(&(file)->f_lock); })
#define fassert_locked(file)  ({ fassert(file); spin_assert_locked(&(file)->f_lock); })

extern int      file_alloc(int *ref, file_t **fref);

extern int      falloc(file_t **pfp);
extern void     fdestroy(file_t *file);
extern int      fdup(file_t *file);
extern int      fput(file_t *file);

extern int      feof(file_t *file);
extern int      fsync(file_t *file);
extern int      fclose(file_t *file);
extern int      funlink(file_t *file);
extern int      fgetattr(file_t *file, void *attr);
extern int      fsetattr(file_t *file, void *attr);
extern int      file_stat(file_t *file, struct stat *buf);
extern int      file_chown(file_t *file, uid_t owner, gid_t group);

extern int      ftruncate(file_t *file, off_t length);
extern int      ffcntl(file_t *file, int cmd, void *argp);
extern int      fioctl(file_t *file, int req, void *argp);
extern off_t    flseek(file_t *file, off_t off, int whence);
extern ssize_t  fread(file_t *file, void *buf, size_t size);
extern ssize_t  fwrite(file_t *file, const void *buf, size_t size);
extern int      fcreate(file_t *dir, const char *filename, mode_t mode);
extern int      fmkdirat(file_t *dir, const char *filename, mode_t mode);
extern ssize_t  freaddir(file_t *dir, off_t off, void *buf, size_t count);
extern int      flinkat(file_t *dir, const char *oldname, const char *newname);
extern int      fmknodat(file_t *dir, const char *filename, mode_t mode, int devid);
extern int      fmmap(file_t *file, vmr_t *region);

extern int      fsymlink(file_t *file, file_t *atdir, const char *symname);
extern int      fbind(file_t *dir, struct dentry *dentry, inode_t *file);

#define NFILE   1024    // file count.

typedef struct file_ctx_t {
    dentry_t    *fc_cwd;      // current working directory of this file context.
    int         fc_fmax;      // file context's allowed maximum for open files.
    dentry_t    *fc_root;     // root directory of this file context.
    int         fc_nfile;     // file context's open file count.
    file_t      **fc_files;   // file context's open file table(array).
    spinlock_t  fc_lock;      // spinlock to guard this file context.
} file_ctx_t;

#define fctx_assert(fctx)        ({ assert(fctx, "No file table pointer."); })
#define fctx_lock(fctx)          ({ fctx_assert(fctx); spin_lock(&(fctx)->fc_lock); })
#define fctx_unlock(fctx)        ({ fctx_assert(fctx); spin_unlock(&(fctx)->fc_lock); })
#define fctx_islocked(fctx)      ({ fctx_assert(fctx); spin_islocked(&(fctx)->fc_lock); })
#define fctx_assert_locked(fctx) ({ fctx_assert(fctx); spin_assert_locked(&(fctx)->fc_lock); })

extern int  fctx_alloc(file_ctx_t**ret);
extern void fctx_free(file_ctx_t *fctx);

extern void file_close_all(void);
extern int  file_copy(file_ctx_t *dst, file_ctx_t *src);

extern int  file_get(int fd, file_t **ref);

extern int  dup(int fd);
extern int  sync(int fd);
extern int  close(int fd);
extern int  unlink(int fd);
extern int  dup2(int fd1, int fd2);
extern int  getattr(int fd, void *attr);
extern int  setattr(int fd, void *attr);
extern int  truncate(int fd, off_t length);
extern int  fcntl(int fd, int cmd, void *argp);
extern int  ioctl(int fd, int req, void *argp);

/* SEEK_SET */
#define SEEK_SET 0
/* SEEK_CUR */
#define SEEK_CUR 1
/* SEEK_END */
#define SEEK_END 2

extern off_t    lseek(int fd, off_t off, int whence);

extern int      chroot(const char *path);
extern int      chdir(const char *path);
extern int      getcwd(char *buf, size_t size);

extern int      isatty(int fd);
extern int      pipe(int fds[2]);
extern mode_t   umask(mode_t cmask);
extern ssize_t  read(int fd, void *buf, size_t size);
extern ssize_t  write(int fd, const void *buf, size_t size);
extern int      mkdir(const char *filename, mode_t mode);
extern int      create(const char *filename, mode_t mode);
extern int      mkdirat(int fd, const char *filename, mode_t mode);
extern int      mknod(const char *filename, mode_t mode, int devid);
extern ssize_t  readdir(int fd, off_t off, void *buf, size_t count);
extern int      open(const char *pathname, int oflags, mode_t mode);
extern int      openat(int fd, const char *pathn, int oflags, mode_t);
extern int      linkat(int fd, const char *oldname, const char *newname);
extern int      openat(int fd, const char *pathname, int oflags, mode_t mode);
extern int      mknodat(int fd, const char *filename, mode_t mode, int devid);
extern int      mount(const char *source, const char *target, const char *type, unsigned long flags, const void *data);