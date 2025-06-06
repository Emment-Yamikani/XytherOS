#include <bits/errno.h>
#include <dev/dev.h>
#include <fs/fs.h>
#include <fs/fcntl.h>
#include <fs/ramfs2.h>
#include <lib/printk.h>
#include <mm/kalloc.h>
#include <string.h>
#include <sys/thread.h>

static iops_t           ramfs2_iops;
static inode_t          *iroot        = NULL;
static superblock_t     *ramfs2_sb    = NULL;
static ramfs2_super_t   *ramfs2_super = NULL;
// static vmr_ops_t ramfs2_vmr_ops __unused;

static int ramfs_fill_sb(filesystem_t *fs, const char *target, struct devid *devid, superblock_t *sb);
static int ramfs_getsb(filesystem_t *fs, const char *src, const char *target,
                        unsigned long flags, void *data, superblock_t **);

filesystem_t ramfs2 = {
    .fs_iops        = &ramfs2_iops,
    .fs_id          = 0,
    .fs_count       = 1,
    .fs_flags       = 0,
    .fs_priv        = NULL,
    .fs_name        = "ramfs",
    .fs_lock        = SPINLOCK_INIT(),
    .get_sb         = ramfs_getsb,
    .fs_superblocks = &QUEUE_INIT(),
};

int ramfs_init(void) {
    int err = 0;
    fslock(&ramfs2);
    if ((err = vfs_register_fs(&ramfs2))) {
        fsunlock(&ramfs2);
        return err;
    }
    fsunlock(&ramfs2);
    return err;
}

static int ramfs_getsb(filesystem_t *fs, const char *src,
        const char *target, unsigned long flags, void *data, superblock_t **psb) {
    return getsb_bdev(fs, src, target, flags, data, psb, ramfs_fill_sb);
}

static int ramfs_fill_sb(filesystem_t *fs, const char *target, struct devid *devid, superblock_t *sb) {
    ssize_t                 err     = 0;
    size_t                  sbsz    = 0;
    ramfs2_super_header_t   hdr     = {0};
    dentry_t                *droot  = NULL;
    ramfs2_node_t           *node   = NULL;

    sbassert_locked(sb);

    if (fs == NULL || devid == NULL || sb == NULL)
        return -EINVAL;

    if ((device_read(devid, 0, &hdr, sizeof hdr)) != sizeof hdr)
        return -EFAULT;

    sbsz = sizeof hdr + (hdr.nfile * sizeof(ramfs2_node_t));

    if ((ramfs2_super = kcalloc(1, sbsz)) == NULL)
        return -ENOMEM;

    if ((err = device_read(devid, 0, ramfs2_super, sbsz)) < 0)
        return err;

    if ((err = ramfs2_validate(ramfs2_super)))
        return err;

    if ((err = ialloc(FS_DIR, I_NORWQUEUES, &iroot)))
        return err;

    if ((err = dalloc(target, &droot))) {
        irelease(iroot);
        return err;
    }

    if ((err = iadd_alias(iroot, droot))) {
        dclose(droot);
        irelease(iroot);
        return err;
    }

    if ((node = kmalloc(sizeof *node)) == NULL) {
        dclose(droot);
        irelease(iroot);
        return err;
    }

    memset(node, 0, sizeof *node);
    strncpy(node->name, droot->d_name, strlen(droot->d_name));

    sb->sb_blocksize = 512;
    sb->sb_size      = ramfs2_super->header.file_size;
    strncpy(sb->sb_magic0, ramfs2_super->header.magic, strlen(ramfs2_super->header.magic));
    sb->sb_uio   = (cred_t) {
        .c_gid   = 0,
        .c_uid   = 0,
        .c_umask = 0555,
        .c_lock  = SPINLOCK_INIT(),
    };

    sb->sb_root     = droot;
    ramfs2_sb       = sb;

    node->offset    = 0;
    node->mode      = 0555;
    node->type      = RAMFS2_DIR;
    node->gid       = sb->sb_uio.c_gid;
    node->uid       = sb->sb_uio.c_uid;

    iroot->i_mode   = node->mode;
    iroot->i_uid    = node->uid;
    iroot->i_gid    = node->gid;
    iroot->i_ino    = node->offset;
    iroot->i_priv   = node;

    iroot->i_type   = FS_DIR;
    iroot->i_sb     = ramfs2_sb;
    iroot->i_ops    = ramfs2_sb->sb_iops;

    dunlock(droot);
    iunlock(iroot);

    return 0;
}

int ramfs2_validate(ramfs2_super_t *super) {
    uint32_t    chksum = 0;
    const char  *magic = "ginger_rd2";
    
    if (!super)
        return -EINVAL;
    if (!string_eq(super->header.magic, (char *)magic))
        return -EINVAL;
    for (int i = 0; i < __magic_len; ++i)
        chksum += (uint32_t)super->header.magic[i];
    chksum += super->header.ramfs2_size + super->header.super_size + super->header.checksum;
    for (uint32_t i = 0; i < super->header.nfile; ++i)
        chksum += super->nodes[i].size;
    return chksum;
}

static ramfs2_node_t *ramfs2_convert_inode(inode_t *ip) {
    if (ip == NULL) return NULL;
    return ip->i_priv;
}

int ramfs2_find(ramfs2_super_t *super, const char *fn, ramfs2_node_t **pnode) {
    if (!super || !pnode)
        return -EINVAL;

    if (!fn || !*fn)
        return -ENOTNAM;

    if ((strlen(fn) >= __max_fname))
        return -ENAMETOOLONG;

    for (uint32_t indx = 0; indx < super->header.nfile; ++indx) {
        if (string_eq(super->nodes[indx].name, (char *)fn)) {
            *pnode = &super->nodes[indx];
            return 0;
        }
    }

    return -ENOENT;
}

static int ramfs_ilookup(inode_t *dir, const char *fname, inode_t **pipp) {
    int             err     = 0;
    itype_t         type    = 0;
    inode_t         *ip     = NULL;
    ramfs2_node_t   *node   = NULL;

    iassert_locked(dir);
    if (!dir || pipp == NULL)
        return -EINVAL;

    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if ((err = ramfs2_find(ramfs2_super, fname, &node)))
        return err;

    type = (int[]){
        [RAMFS2_INV] = FS_INV,
        [RAMFS2_REG] = FS_RGL,
        [RAMFS2_DIR] = FS_DIR,
    }[node->type];

    if ((err = ialloc(type, 0, &ip)))
        return err;

    ip->i_priv  = node;
    ip->i_type  = type;
    ip->i_sb    = ramfs2_sb;
    ip->i_gid   = node->gid;
    ip->i_uid   = node->uid;
    ip->i_mode  = node->mode;
    ip->i_size  = node->size;
    ip->i_ops   = ramfs2_sb->sb_iops;
    ip->i_ino   = node - ramfs2_super->nodes;

    *pipp = ip;
    return 0;
};

static int ramfs2_open(inode_t *ip __unused, inode_t **pip __unused) {
    return 0;
}

static ssize_t ramfs2_read_data(inode_t *ip, off_t off, void *buf, size_t sz) {
    ssize_t         retval = 0;
    ramfs2_node_t   *node  = NULL;
    if (!ip || !buf)
        return -EINVAL;
    if ((node = ramfs2_convert_inode(ip)) == NULL)
        return -EINVAL;
    if (off >= node->size)
        return -1;
    sz = MIN((node->size - off), sz);
    off += node->offset + ramfs2_super->header.data_offset;
    
    sblock(ip->i_sb);
    retval = device_read(&ip->i_sb->sb_devid, off, buf, sz);
    sbunlock(ip->i_sb);
    return retval;
}

static ssize_t ramfs2_write_data(inode_t *ip __unused, off_t off __unused, void *buf __unused, size_t sz __unused) {
    return -EROFS;
}

static int ramfs2_close(inode_t *ip __unused) {
    return 0;
}

static int ramfs2_creat(inode_t *ip __unused, const char *fname __unused, int mode __unused) {
    return -EROFS;
}

static int ramfs2_sync(inode_t *ip __unused) {
    return -EROFS;
}

static int ramfs2_ioctl(inode_t *ip __unused, int req __unused, void *argp __unused) {
    return -ENOTTY;
}

__unused static int ramfs2_lseek(inode_t *ip __unused, off_t off __unused, int whence __unused) {
    return -EINVAL;
}

static ssize_t ramfs2_readdir(inode_t *dir, off_t offset, struct dirent *buff, size_t count) {
    ramfs2_node_t *node = NULL;
    
    iassert_locked(dir);

    if (iroot != dir)
        return -EINVAL;
    
    if (IISDIR(dir) == 0)
        return -ENOTDIR;

    if (offset >= ramfs2_super->header.nfile)
        return -1;

    if ((node = ramfs2_convert_inode(dir)) == NULL)
        return -EINVAL;

    for (uint32_t indx = 0; indx < ramfs2_super->header.nfile; ++indx) {
        if (indx >= count)
            break;
        buff[indx] = (struct dirent){
            .d_off  = offset,
            .d_ino  = offset + 1,
            .d_reclen = sizeof *buff,
            .d_size = ramfs2_super->nodes[offset].size,
            .d_type = (int[]) {
                [RAMFS2_INV] = 0,
                [RAMFS2_REG] = _IFREG,
                [RAMFS2_DIR] = _IFDIR,
            }[ramfs2_super->nodes[offset].type],
            .d_name[0] = '\0',
        };

        switch (ramfs2_super->nodes[offset].type) {
        case RAMFS2_DIR:
            buff[indx].d_type = FS_DIR;
            break;
        case RAMFS2_REG:
            buff[indx].d_type = FS_RGL;
            break;
        default:
            buff[indx].d_type = FS_INV;
        }

        strncpy(buff[indx].d_name, ramfs2_super->nodes[offset].name,
            strlen(ramfs2_super->nodes[offset].name));
    }

    return 0;
}

__unused static int ramfs2_chown(inode_t *ip __unused, uid_t uid __unused, gid_t gid __unused) {
    return -EROFS;
}

/*
int ramfs2_fault(vmr_t *vmr __unused, struct vm_fault *fault __unused) {
    return -ENOSYS;
}

int ramfs2_mmap(file_t *file __unused, vmr_t *vmr __unused) {
    return -ENOSYS;
}
*/

static iops_t ramfs2_iops = {
    .ibind      = NULL,
    .iopen      = ramfs2_open,
    .isync      = ramfs2_sync,
    // .ilseek     = ramfs2_lseek,
    .ichown     = ramfs2_chown,
    .iioctl     = ramfs2_ioctl,
    .iclose     = ramfs2_close,
    .icreate    = ramfs2_creat,
    .ilookup    = ramfs_ilookup,
    .ireaddir   = ramfs2_readdir,
    .iread      = ramfs2_read_data,
    .iwrite     = ramfs2_write_data,
};