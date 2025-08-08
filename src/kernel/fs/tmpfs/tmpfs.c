#include <bits/errno.h>
#include <core/debug.h>
#include <dev/dev.h>
#include <ds/btree.h>
#include <ds/stack.h>
#include <fs/devtmpfs.h>
#include <fs/fs.h>
#include <fs/stat.h>
#include <fs/tmpfs.h>
#include <mm/kalloc.h>
#include <ds/hashMap.h>

typedef struct tmpfs_inode_t {
    uid_t       uid;
    gid_t       gid;
    mode_t      mode;
    uintptr_t   ino;
    itype_t     type;
    dev_t       rdev;
    size_t      size;
    long        hlink;
    void        *data;
} tmpfs_inode_t;

typedef struct tmpfs_dirent_t {
    char                    *name;
    tmpfs_inode_t           *inode;
} tmpfs_dirent_t;

int tmpfs_init(void);
int tmpfs_new_inode(itype_t type, inode_t **pip);
static int tmpfs_ialloc(itype_t type, tmpfs_inode_t **pip);

#define tmpfs_data(ip) ({       \
    void *data = NULL;          \
    tmpfs_inode_t *tino = NULL; \
    if ((tino = ip->i_priv))    \
        data = tino->data;      \
    data;                       \
})

static fs_t *tmpfs = NULL;

static iops_t tmpfs_iops = {
    .iopen      = tmpfs_iopen,
    .ibind      = tmpfs_ibind,
    .isync      = tmpfs_isync,
    .ilink      = tmpfs_ilink,
    .iread      = tmpfs_iread,
    .iwrite     = tmpfs_iwrite,
    .iclose     = tmpfs_iclose,
    .ifcntl     = tmpfs_ifcntl,
    .iioctl     = tmpfs_iioctl,
    .imkdir     = tmpfs_imkdir,
    .imknod     = tmpfs_imknod,
    .icreate    = tmpfs_icreate,
    .ilookup    = tmpfs_ilookup,
    .iunlink    = tmpfs_iunlink,
    .irename    = tmpfs_irename,
    .igetattr   = tmpfs_igetattr,
    .isetattr   = tmpfs_isetattr,
    .isymlink   = tmpfs_isymlink,
    .ireaddir   = tmpfs_ireaddir,
    .itruncate  = tmpfs_itruncate,
};

static hashKey tmpfs_hash(hashMap *, const void *key) {
    int     c   = 0;
    char    *str= (char *)key;
    hashKey hash= 5381;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static int tmpfs_dump_hash(hashMap *, hashEntry *entry, void *) {
    if (entry == NULL) {
        return -EINVAL;
    }

    tmpfs_dirent_t *dirent = (tmpfs_dirent_t *)entry->value;
    printk("File Name: %s\n", dirent->name);

    return 0;
}

static hashMapContext tmpfs_hashctx = {
    .compare    = NULL,
    .copy       = NULL,
    .destroy    = NULL,
    .hash       = tmpfs_hash,
    .dump       = tmpfs_dump_hash,
};

static int tmpfs_fill_sb(fs_t *, const char *target, struct devid *, sblock_t *sb) {
    int         err    = 0;
    inode_t     *iroot = NULL;
    dentry_t    *droot = NULL;

    if ((err = tmpfs_new_inode(FS_DIR, &iroot))) {
        return err;
    }
    
    if ((err = dalloc(target, &droot))) {
        irelease(iroot);
        return err;
    }

    if ((err = iadd_alias(iroot, droot))) {
        dclose(droot);
        irelease(iroot);
        return err;
    }
    
    sb->sb_blocksize = -1;
    strncpy(sb->sb_magic0, "virtual FS", 10);
    sb->sb_size = -1;
    sb->sb_uio  = (cred_t){
        .c_gid  = 0,
        .c_uid  = 0,
        .c_umask= 0555,
        .c_lock = SPINLOCK_INIT(),
    };
    sb->sb_root = droot;

    iroot->i_sb = sb;
    iroot->i_type = FS_DIR;
    iroot->i_ops = sb->sb_iops;
    dunlock(droot);
    iunlock(iroot);
    return 0;
}

static int tmpfs_getsb(fs_t *fs, const char *, const char *target, ulong flags, void *data, sblock_t **psbp) {
    return getsb_nodev(fs, target, flags, data, psbp, tmpfs_fill_sb);
}


static inline int tmpfs_validate_dir(inode_t *dir) {
    if (!dir || !dir->i_priv || !tmpfs_data(dir)) {
        return -EINVAL;
    }

    if (IISDIR(dir) == 0) {
        return -ENOTDIR;
    }

    return 0;
}

static inline int tmpfs_dirent_type(tmpfs_dirent_t *dirent) {
    int d_type = 0;
    switch (dirent->inode->type) {
    case FS_DIR:  d_type = DT_DIR;  break;
    case FS_RGL:  d_type = DT_REG;  break;
    case FS_BLK:  d_type = DT_BLK;  break;
    case FS_CHR:  d_type = DT_CHR;  break;
    case FS_FIFO: d_type = DT_FIFO; break;
    case FS_INV:
    default:      d_type = 0;       break;
    }
    return d_type;
} 

int tmpfs_init(void) {
    int err = 0;

    if ((err = fs_create("tmpfs", &tmpfs_iops, &tmpfs))) {
        return err;
    }

    tmpfs->get_sb = tmpfs_getsb;
    tmpfs->mount = NULL;

    if ((err = vfs_register_fs(tmpfs))) {
        fsunlock(tmpfs);
        goto error;
    }

    fsunlock(tmpfs);
    return 0;
error:
    if (tmpfs) {
        fs_free(tmpfs);
    }
    return err;
}

static int tmpfs_ialloc(itype_t type, tmpfs_inode_t **pipp) {
    int             err         = 0;
    tmpfs_inode_t   *ip         = NULL;
    hashMap         *hmap       = NULL;
    static long     tmpfs_inos  = 0;

    if (pipp == NULL) {
        return -EINVAL;
    }
    
    if ((ip = kzalloc(sizeof *ip)) == NULL) {
        return -ENOMEM;
    }

    if (type == FS_DIR) {
        if ((err = hashMap_alloc(&tmpfs_hashctx, &hmap))) {
            goto error;
        }
    }

    ip->hlink   = 1;
    ip->type    = type;
    ip->data    = hmap;
    ip->ino     = atomic_fetch_inc(&tmpfs_inos);

    *pipp = ip;

    return 0;
error:
    if (ip)
        kfree(ip);
    if (hmap)
        hashMap_free(hmap);
    return err;
}

static int tmpfs_ifree(tmpfs_inode_t *ip) {
    if (ip == NULL) {
        return -EINVAL;
    }

    if (--ip->hlink <= 0) {
        if (ip->type == FS_DIR) {
            hashMap *hmap = ip->data;

            if (!hmap) {
                return -EINVAL;
            }

            hashMap_free(hmap);
        } else if (ip->data) {
            kfree(ip->data);
        }
    }

    return 0;
}

int tmpfs_new_inode(itype_t type, inode_t **pip) {
    int err;
    inode_t *inode = NULL;
    tmpfs_inode_t *tmpfs_ip = NULL;

    if (pip == NULL) {
        return -EINVAL;
    }

    if ((err = tmpfs_ialloc(type, &tmpfs_ip))) {
        return err;
    }

    if ((err = ialloc(type, 0, &inode))) {
        tmpfs_ifree(tmpfs_ip);
        return err;
    }

    inode->i_type   = type;
    inode->i_priv   = tmpfs_ip;
    inode->i_ops    = &tmpfs_iops;
    inode->i_ino    = tmpfs_ip->ino;

    *pip = inode;
    return 0;
}

static int tmpfs_dirent_alloc(const char *fname, tmpfs_inode_t *ip, tmpfs_dirent_t **ptde) {
    if (!fname || !ptde) {
        return -EINVAL;
    }
    
    char *name = strdup(fname);
    if (!name) {
        return -ENOMEM;
    }

    tmpfs_dirent_t *tde = (tmpfs_dirent_t *)kmalloc(sizeof (tmpfs_dirent_t));
    if (tde == NULL) {
        kfree(name);
        return -ENOMEM;
    }

    tde->inode  = ip;
    tde->name   = name;

    *ptde = tde;
    return 0;
}

static int tmpfs_dirent_free(tmpfs_dirent_t *dirent) {
    if (dirent == NULL) {
        return -EINVAL;
    }
    
    if (dirent->inode) {
        dirent->inode->hlink--;
    }

    if (dirent->name) {
        kfree(dirent->name);
    }

    kfree(dirent);
    return 0;
}

static int tmpfs_create_node(inode_t *dir, const char *fname, mode_t mode, itype_t type, dev_t dev) {
    int             err     = 0;
    tmpfs_inode_t   *ip     = NULL;
    tmpfs_dirent_t  *dirent = NULL;

    iassert_locked(dir);

    if ((err = tmpfs_validate_dir(dir))) {
        return err;
    }

    if ((err = tmpfs_ialloc(type, &ip))) {
        return err;
    }

    if ((err = tmpfs_dirent_alloc(fname, ip, &dirent))) {
        tmpfs_ifree(ip);
        return err;
    }

    hashMap *hmap = (hashMap *)tmpfs_data(dir);

    hashMap_lock(hmap);
    err = hashMap_insert(hmap, fname, dirent);
    hashMap_unlock(hmap);

    if (err != 0) {
        tmpfs_dirent_free(dirent);
        tmpfs_ifree(ip);
        return err;
    }

    ip->mode = mode;
    ip->rdev = dev;

    // printk("%s:%d: [\e[0;04mWARNING\e[0m]: file credentials not fully set!: rdev[%d:%d]\n",
        //    __FILE__, __LINE__, dev & 0xff, dev >> 8);
    return 0;
}

int tmpfs_imkdir(inode_t *dir, const char *fname, mode_t mode) {
    return tmpfs_create_node(dir, fname, mode, FS_DIR, 0);
}

int tmpfs_icreate(inode_t *dir, const char *fname, mode_t mode) {
    return tmpfs_create_node(dir, fname, mode, FS_RGL, 0);
}

int tmpfs_iopen(inode_t *ip __unused, inode_t **pip __unused) {
    return 0;
}

int tmpfs_ilookup(inode_t *dir, const char *fname, inode_t **pipp) {
    int             err     = 0;
    inode_t         *ip     = NULL;
    tmpfs_dirent_t  *dirent = NULL;
    
    iassert_locked(dir);
    
    if ((err = tmpfs_validate_dir(dir))) {
        return err;
    }

    if (!pipp) {
        return -EINVAL;
    }
    
    hashMap *hmap = (hashMap *)tmpfs_data(dir);

    // printk("tmpfs looking-up file(\e[0;013m%s\e[0m).\n", fname);
 
    hashMap_lock(hmap);
    err = hashMap_lookup(hmap, fname, (void **)&dirent);
    hashMap_unlock(hmap);

    if (err != 0) {
        return err;
    }

    // printk("tmpfs found file(\e[0;013m%s\e[0m).\n", fname);

    if (dirent->inode == NULL) {
        return -EINVAL;
    }
    
    if ((err = ialloc(dirent->inode->type, 0, &ip))) {
        return err;
    }
    
    ip->i_ops = IISDEV(ip) ? &dev_iops : dir->i_ops;

    // printk("file_type: %s\n", itype_strings[dirent->inode->type]);

    ip->i_priv      = dirent->inode;
    ip->i_ino       = dirent->inode->ino;
    ip->i_type      = dirent->inode->type;
    ip->i_size      = dirent->inode->size;
    ip->i_hlinks    = dirent->inode->hlink;
    ip->i_gid       = dirent->inode->gid;
    ip->i_uid       = dirent->inode->uid;
    ip->i_mode      = dirent->inode->mode;
    ip->i_rdev      = dirent->inode->rdev;

    // printk("%s:%d: rdev[%d:%d]\n",
        // __FILE__, __LINE__, ip->i_rdev & 0xff, ip->i_rdev >> 8);
    *pipp = ip;    
    return 0;
}

int tmpfs_itruncate(inode_t *ip) {
    void *data = NULL;
    tmpfs_inode_t *tino = NULL;

    iassert_locked(ip);
    if (ip == NULL || IISDEV(ip)) {
        return -EINVAL;
    }
    
    if (IISDIR(ip)) {
        return -EISDIR;
    }

    if ((tino = ip->i_priv) == NULL) {
        return -EINVAL;
    }

    if ((data = tmpfs_data(ip)) == NULL) {
        return 0;
    }
    
    kfree(data);

    ip->i_size = 0;
    tino->size = 0;
    tino->data = NULL;

    return 0;
}

ssize_t tmpfs_iread(inode_t *ip, off_t off, void *buf, size_t sz) {
    iassert_locked(ip);
    
    if (ip == NULL || buf == NULL) {
        return -EINVAL;
    }
    
    if (IISDIR(ip)) {
        return -EISDIR;
    }
    
    void *data = tmpfs_data(ip);
    tmpfs_inode_t *tino = ip->i_priv;

    if (tino == NULL) {
        return -EINVAL;
    }

    if ((off >= tino->size) || data == NULL) {
        return -1;
    }

    sz = MIN((tino->size - off), sz);
    memcpy(buf, ((const char *)data + off), sz);

    return sz;
}

ssize_t tmpfs_iwrite(inode_t *ip, off_t off, const void *buf, size_t sz) {
    void *data = NULL;
    tmpfs_inode_t *tino = NULL;

    iassert_locked(ip);

    if (ip == NULL || buf == NULL)
        return -EINVAL;

    if (IISDIR(ip))
        return -EISDIR;

    if ((tino = ip->i_priv) == NULL)
        return -EINVAL;

    if ((off + sz) > tino->size) {
        if (tino->data == NULL) {
            if ((data = kmalloc(off + sz)) == NULL)
                return -EAGAIN;
        } else {
            if ((data = krealloc(tino->data, off + sz)) == NULL)
                return -EAGAIN;
        }

        tino->data = data;
        tino->size = off + sz;
        ip->i_size = tino->size;
    }

    sz = MIN((tino->size - off), sz);
    memcpy((char *)data + off, buf, sz);
    return sz;
}

int tmpfs_isync(inode_t *ip __unused) {
    return -ENOTSUP;
}

int tmpfs_iclose(inode_t *ip __unused) {
    return -ENOTSUP;
}

int tmpfs_iunlink(inode_t *ip __unused) {
    return -ENOTSUP;
}

int tmpfs_igetattr(inode_t *ip __unused, void *attr __unused) {
    return -ENOTSUP;
}

int tmpfs_isetattr(inode_t *ip __unused, void *attr __unused) {
    return -ENOTSUP;
}

int tmpfs_ifcntl(inode_t *ip __unused, int cmd __unused, void *argp __unused) {
    return -ENOTSUP;
}

int tmpfs_iioctl(inode_t *ip, int req __unused, void *argp) {
    iassert_locked(ip);
    
    if (ip == NULL || argp == NULL)
        return -EINVAL;
    
    if (IISDEV(ip) == 0)
        return -ENOTTY;

    return -ENOTSUP;
}

int tmpfs_ibind(inode_t *dir __unused, dentry_t *dentry __unused, inode_t *ip __unused) {
    return -ENOTSUP;
}

int tmpfs_isymlink(inode_t *ip __unused, inode_t *atdir __unused, const char *symname __unused) {
    return -ENOTSUP;
}

/* Read directory entries into buffer from tmpfs directory
 * Args:
 *   dir   - Locked directory inode to read from
 *   off   - Offset in directory entries to start reading
 *   buf   - User buffer to store directory entries
 *   count - Maximum number of entries to read
 * Returns:
 *   Number of entries read on success, negative error code on failure
 */
ssize_t tmpfs_ireaddir(inode_t *dir, off_t off, struct dirent *buf, size_t count) {
    int err = 0;

    // Verify caller holds directory lock
    iassert_locked(dir);

    // Validate directory structure and permissions
    if ((err = tmpfs_validate_dir(dir))) {
        return err;
    }

    // Check output buffer requirements
    if (!buf || !count) {
        return -EINVAL;
    }

    // Get directory's hash table of entries
    hashMap  *hmap = (hashMap *)tmpfs_data(dir);

    if (hmap == NULL) {
        return 0; // EOF
    }

    // Lock hash table while traversing entries
    hashMap_lock(hmap);

    // Get total number of directory entries
    const usize hmap_size = hashMap_size(hmap);
    // Validate requested offset against actual entries
    if (off >= hmap_size) {
        hashMap_unlock(hmap);
        return -EFAULT;
    }

    ITER(iter);

    size_t pos = 0, ncount = 0;

    // Iterate through entries with count-limited loop
    tmpfs_dirent_t *dirent;
    hashMap_foreach_item(hmap, dirent, iter) {
        // Skip entries until we reach requested offset
        if (pos++ < off) continue;

        // Prepare destination buffer entry
        struct dirent  *dbuf    = &buf[ncount];
        // Populate directory entry structure
        *dbuf = (struct dirent) {
            .d_off      = off + ncount,              // Logical offset in directory
            .d_ino      = dirent->inode->ino,        // Inode number
            .d_size     = dirent->inode->size,       // File size
            .d_reclen   = sizeof (struct dirent),    // Record length
            .d_type     = tmpfs_dirent_type(dirent), // File type
        };

        /* Copy filename with safety measures:
            * 1. strncpy prevents buffer overflow
            * 2. Explicit null termination ensures valid C string */
        safestrncpy(dbuf->d_name, dirent->name, sizeof(dbuf->d_name) - 1);
        // dbuf->d_name[sizeof(dbuf->d_name) - 1] = '\0';

        ncount++;  // Track number of successfully copied entries
    }

    hashMap_unlock(hmap);

    // Return actual number of entries copied to user buffer
    return ncount;
}

int tmpfs_ilink(const char *oldname __unused, inode_t *dir __unused, const char *newname __unused) {
    return -ENOTSUP;
}

int tmpfs_imknod(inode_t *dir, const char *name, mode_t mode, int devid) {
    // Validate the mode
    if ((mode & S_IFMT) == 0) {
        mode |= S_IFREG; // Default to regular file
    }

    return tmpfs_create_node(dir, name, mode, iextract_type(mode), devid);
}

int tmpfs_irename(inode_t *dir __unused, const char *old __unused, inode_t *newdir __unused, const char *new __unused) {
    return -ENOTSUP;
}
