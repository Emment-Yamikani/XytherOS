#pragma once

#include <core/types.h>
#include <ds/queue.h>
#include <fs/cred.h>
#include <fs/dentry.h>
#include <fs/fcntl.h>
#include <fs/inode.h>
#include <fs/mount.h>
#include <fs/path.h>
#include <fs/stat.h>
#include <sync/spinlock.h>

#define MAXFNAME 255

struct dirent {
    size_t d_ino;
    int    d_type;
    off_t  d_off;
    size_t d_reclen;
    size_t d_size;
    char   d_name[MAXFNAME];
};

/* Directory entry type macros (d_type values) */
#define DT_UNKNOWN 0 // Unknown file type
#define DT_FIFO 1    // Named pipe (FIFO)
#define DT_CHR 2     // Character device
#define DT_DIR 4     // Directory
#define DT_BLK 6     // Block device
#define DT_REG 8     // Regular file
#define DT_LNK 10    // Symbolic link
#define DT_SOCK 12   // Unix domain socket

struct filesystem;
struct devid;
struct superblock;

typedef struct {
    dentry_t *(*mount)(struct superblock *);
    inode_t *(*inode_alloc)(struct superblock *);
    int (*unmount)(struct superblock *);
    int (*getattr)(struct superblock *);
    int (*setattr)(struct superblock *);
} sb_ops_t;

typedef struct superblock {
    long                sb_id;
    cred_t              sb_uio;
    sb_ops_t            sb_ops;
    iops_t              *sb_iops;
    long                sb_count;
    uintptr_t           sb_magic;
    uintptr_t           sb_flags;
    uintptr_t           sb_iflags;
    void                *sb_priv;
    struct devid        sb_devid;
    dentry_t            *sb_root;
    size_t              sb_size;
    char                sb_magic0[31];
    size_t              sb_blocksize;
    fs_mount_t          *sb_mnt;
    struct filesystem   *sb_filesystem;
    spinlock_t          sb_lock;
} superblock_t;

#define sbassert(sb)        ({ assert((sb), "No superblock"); })
#define sblock(sb)          ({ sbassert(sb); spin_lock(&(sb)->sb_lock); });
#define sbunlock(sb)        ({ sbassert(sb); spin_unlock(&(sb)->sb_lock); })
#define sblocked(sb)        ({ sbassert(sb); spin_locked(&(sb)->sb_lock); })
#define sbassert_locked(sb) ({ sbassert(sb); spin_assert_locked(&(sb)->sb_lock); })


typedef struct filesystem {
    long        fs_id;
    long        fs_flags;
    long        fs_count;
    char        *fs_name;
    iops_t      *fs_iops;
    void        *fs_priv;
    queue_t     *fs_superblocks;
    int         (*remount)(filesystem_t *fs, fs_mount_t *mnt, u64 flags, const void *data);
    int         (*get_sb)(struct filesystem *fs, const char *src, const char *target, unsigned long flags, void *data, superblock_t **psbp);
    int         (*mount)(struct filesystem *fs, dentry_t *src, dentry_t *dst, unsigned long flags, void *data);
    spinlock_t  fs_lock;
} filesystem_t;

#define fsassert(fs)        ({ assert((fs), "No filesystem"); })
#define fslock(fs)          ({ fsassert(fs); spin_lock(&(fs)->fs_lock); });
#define fsunlock(fs)        ({ fsassert(fs); spin_unlock(&(fs)->fs_lock); })
#define fsislocked(fs)      ({ fsassert(fs); spin_islocked(&(fs)->fs_lock); })
#define fsassert_locked(fs) ({ fsassert(fs); spin_assert_locked(&(fs)->fs_lock); })

void fs_dup(filesystem_t *fs);
void fs_put(filesystem_t *fs);
void fs_free(filesystem_t *fs);
long fs_count(filesystem_t *fs);
void fs_unsetname(filesystem_t *fs);
int  fs_set_iops(filesystem_t *fs, iops_t *iops);
int  fs_setname(filesystem_t *fs, const char *fsname);
int  fs_create(const char *name, iops_t *iops, filesystem_t **pfs);
int  fs_add_superblock(filesystem_t *fs, superblock_t *sb);
int  fs_del_superblock(filesystem_t *fs, superblock_t *sb);


int vfs_init(void);
int ramfs_init(void);
int vfs_alloc_vnode(const char *name, itype_t type, inode_t **pip, dentry_t **pdp);
dentry_t *vfs_getdroot(void);
int vfs_mount_droot(dentry_t *dentry);
int vfs_register_fs(filesystem_t *fs);
int vfs_unregister_fs(filesystem_t *fs);
int  vfs_getfs(const char *type, filesystem_t **pfs);

int vfs_traverse_path(vfspath_t *path, cred_t *cred, int oflags);
int vfs_resolve_path(const char *pathname, dentry_t *dir, cred_t *cred, int oflags, vfspath_t **dp);

int vfs_lookup(const char *fn, cred_t *cred, int oflags, dentry_t **pdp);
int vfs_lookupat(const char *pathname, dentry_t *dir, cred_t *__cred, int oflags, dentry_t **pdp);

int vfs_mknod(const char *pathname, cred_t *cred, mode_t mode, dev_t dev);
int vfs_mknodat(dentry_t *dir, const char *pathname, cred_t *cred, mode_t mode, dev_t dev);

int vfs_mkdir(const char *path, cred_t *cred, mode_t mode);
int vfs_mkdirat(const char *pathname, dentry_t *dir, cred_t *cred, mode_t mode);

int vfs_dirlist(const char *path);

/**
 * VFS mount helpers
*/

fs_mount_t *alloc_fsmount(void);

/**
 * Super block helpers
*/

int getsb_bdev(filesystem_t *fs,    const char *bdev_name,
    const char *target, unsigned long flags, void *data,
    superblock_t **psbp, int (*sb_fill)(filesystem_t *fs,
    const char *target, struct devid *dd, superblock_t *sb));

int getsb_nodev(filesystem_t *fs,   const char *target,
    unsigned long flags, void *data, superblock_t **psbp,
    int (*sb_fill)(filesystem_t *fs, const char *target,
    struct devid *dd, superblock_t *sb));

int getsb_pseudo(filesystem_t *fs,  const char *target,
    unsigned long flags, void *data, superblock_t **psbp,
    int (*sb_fill)(filesystem_t *fs, const char *target,
    struct devid *dd, superblock_t *sb));

int getsb_single(filesystem_t *fs,  const char *target,
    unsigned long flags, void *data, superblock_t **psbp,
    int (*sb_fill)(filesystem_t *fs, const char *target,
    struct devid *dd, superblock_t *sb));