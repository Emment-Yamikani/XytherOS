#include <bits/errno.h>
#include <fs/fs.h>
#include <dev/dev.h>
#include <mm/kalloc.h>
#include <string.h>

static atomic_t sbID = 0;

#define NODEV_TMPFS 0
#define NODEV_DEVFS 1

int getsb(fs_t * fs, devid_t *devid, sblock_t **psb) {
    int err = 0;
    sblock_t *sb = NULL;
    queue_node_t *next = NULL;

    fsassert_locked(fs);

    if (psb == NULL || devid == NULL) {
        return -EINVAL;
    }

    queue_lock(fs->fs_superblocks);
    forlinked(node, fs->fs_superblocks->head, next) {
        next = node->next;
        sb = node->data;
        sblock(sb);
        if (DEVID_CMP(devid, &sb->sb_devid)) {
            *psb = sb;
            queue_unlock(fs->fs_superblocks);
            return 0;
        }
        sbunlock(sb);
    }
    queue_unlock(fs->fs_superblocks);

    if ((sb = kzalloc(sizeof *sb)) == NULL) {
        return -ENOMEM;
    }

    sb->sb_lock = SPINLOCK_INIT();

    *sb = (sblock_t) {
        .sb_count       = 1,
        .sb_blocksize   = 0,
        .sb_flags       = 0,
        .sb_filesystem  = fs,
        .sb_priv        = NULL,
        .sb_devid       = *devid,
        .sb_id          = atomic_inc_fetch(&sbID),
    };

    sblock(sb);

    if ((err = fs_add_superblock(fs, sb))) {
        sbunlock(sb);
        kfree(sb);
        return err;
    }
    
    *psb = sb;
    return 0;
}

int getsb_bdev(fs_t *fs, const char *bdev_name, const char *target, usize flags,
    void *data __unused, sblock_t **psbp, sb_fill_fn_t sb_fill) {
    int             err     = 0;
    sblock_t    *sb     = NULL;
    devid_t         devid   = {0};
    bdev_info_t     bdevinfo= {0};

    fsassert_locked(fs);

    if (psbp == NULL) {
        return -EINVAL;
    }

    if (sb_fill == NULL) {
        return -ENOSYS;
    }

    if ((err = find_bdev_by_name(bdev_name, &devid))) {
        return err;
    }
    
    if ((err = getsb(fs, &devid, &sb))) {
        if (err == -EINVAL) {
            return err;
        }
    }

    device_getinfo(&devid, &bdevinfo);

    sb->sb_flags        |= flags;
    sb->sb_size         = bdevinfo.bi_size;
    sb->sb_blocksize    = bdevinfo.bi_blocksize;

    if ((err = sb_fill(fs, target, &devid, sb))) {
        return err;
    }

    *psbp = sb;
    return 0;
}

int getsb_nodev(fs_t *fs, const char *target, usize flags __unused,
    void *data __unused, sblock_t **psbp, sb_fill_fn_t sb_fill) {
    int err = 0;
    
    if ((sb_fill == NULL)) {
        return -ENOSYS;
    }
    
    if (psbp == NULL) {
        return -EINVAL;
    }
    
    static uint16_t devno = 1;
    uint16_t minor = atomic_fetch_inc(&devno);

    if (minor >= 256) {
        return -ENOSPC;
    }

    char *name;

    char dev_minor[8] = {0};
    snprintf(dev_minor, sizeof dev_minor - 1, "%d", minor);
    if ((name = combine_strings("virtdev", dev_minor)) == NULL) {
        return -ENOMEM;
    }

    device_t *dev;
    if ((err = device_create(name, FS_BLK, 0, NULL, &dev))) {
        return err;
    }

    sblock_t *sb;
    if ((err = getsb(fs, &dev->devid, &sb))) {
        dev_unlock(dev);
        return err;
    }

    if ((err = sb_fill(fs, target, &dev->devid, sb))) {
        dev_unlock(dev);
        return err;
    }

    dev_unlock(dev);
    *psbp = sb;
    return 0;
}

int getsb_pseudo(fs_t *fs, const char *target, usize flags,
    void *data, sblock_t **psbp, sb_fill_fn_t sb_fill);

int getsb_single(fs_t *fs, const char *target, usize flags,
    void *data, sblock_t **psbp, sb_fill_fn_t sb_fill);
