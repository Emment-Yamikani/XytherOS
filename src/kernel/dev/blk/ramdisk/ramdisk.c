#include <bits/errno.h>
#include <boot/boot.h>
#include <core/debug.h>
#include <dev/dev.h>
#include <mm/kalloc.h>
#include <string.h>

#define NRAMDISK        8   // total number of disks allowed.
#define RAMDISK_GETINFO 0   // ioctl to get ramdisk info.

DECL_DEVOPS(static, ramdisk);   // declaretion of ramdisk devops.

typedef struct ramdisk {
    void        *data;
    usize       size;
    spinlock_t  lock;
} ramdisk_t;

#define disk_lock(r)             ({ spin_lock(&(r)->lock); })
#define disk_unlock(r)           ({ spin_unlock(&(r)->lock); })
#define disk_islocked(r)         ({ spin_islocked(&(r)->lock); })
#define disk_recursive_lock(r)   ({ spin_recursive_lock(&(r)->lock); })
#define disk_assert_locked(r)    ({ spin_assert_locked(&(r)->lock); })

static atomic_t  disk_count = 0;
static ramdisk_t *disks[NRAMDISK];

static SPINLOCK(disks_lock);
#define ramdisks_lock()             ({ spin_lock(disks_lock); })
#define ramdisks_unlock()           ({ spin_unlock(disks_lock); })
#define ramdisks_assert_locked()    ({ spin_assert_locked(disks_lock); })

static int disk_alloc(ramdisk_t **prp) {
    ramdisk_t *disk;

    if (prp == NULL) {
        return -EINVAL;
    }

    if (NULL == (disk = kzalloc(sizeof *disk))) {
        return -ENOMEM;
    }

    spinlock_init(&disk->lock);

    *prp = disk;
    return 0;
}

static void disk_free(ramdisk_t *disk) {
    if (disk == NULL) {
        return;
    }

    disk_recursive_lock(disk);
    disk_unlock(disk);

    kfree(disk);
}

static ramdisk_t *get_ramdisk(devid_t *dd) {
    if (!valid_devno(dd->minor, NRAMDISK) || dd->type != BLKDEV || dd->major != RAMDISK_DEV_MAJOR) {
        return NULL;
    }

    ramdisks_lock();
    ramdisk_t *disk = disks[dd->minor];
    ramdisks_unlock();

    return disk;
}

static int ramdisk_init(void) {
    mod_t *mod = (mod_t *)bootinfo.mods;

    memset(disks, 0, sizeof disks);

    if (bootinfo.modcnt > NRAMDISK) {
        return -EAGAIN;
    }

    for (u32 i = 0; i < bootinfo.modcnt; ++i, mod++) {
        int         err;
        device_t    *dev;
        ramdisk_t   *disk;
        char        name[16];

        if ((err = disk_alloc(&disk))) {
            return err;
        }

        name[0] = '\0';

        snprintf(name, sizeof name, "ramdisk%d", i);

        if ((err = device_create(name, BLKDEV, RAMDISK_DEV_MAJOR, DEVOPS_PTR(ramdisk), &dev))) {
            disk_free(disk);
            return err;
        }

        if ((err = device_register(dev))) {
            device_destroy(dev);
            disk_free(disk);
            return err;
        }

        disk->size = mod->size;
        disk->data = (void *) mod->addr;

        ramdisks_lock();
        disks[DEVID_MINOR(&dev->devid)] = disk;
        ramdisks_unlock();

        atomic_inc(&disk_count);

        dev_unlock(dev);
    }

    return 0;
} BUILTIN_DEVICE(ramdisk, ramdisk_init, NULL, NULL);

static int ramdisk_probe(devid_t *dd __unused) {
    return 0;
}

static int ramdisk_fini(devid_t *dd __unused) {
    return 0;
}

static int ramdisk_close(devid_t *dd __unused) {
    return 0;
}

static int ramdisk_open(devid_t *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int ramdisk_getinfo(devid_t *dd, void *info) {
    return ramdisk_ioctl(dd, RAMDISK_GETINFO, info);
}

static int ramdisk_mmap(devid_t *dd __unused, vmr_t *vmregion __unused) {
    return -ENOSYS;
}

static int ramdisk_ioctl(devid_t *dd, int request, void *arg) {
    ramdisk_t *disk = get_ramdisk(dd);
    if (disk == NULL || arg == NULL) {
        return -EINVAL;
    }

    int     err     = 0;
    bool    locked  = disk_recursive_lock(disk);

    switch (request) {
        case RAMDISK_GETINFO:
            *((bdev_info_t *)arg) = (bdev_info_t) { .bi_blocksize = 1, .bi_size = disk->size };
            break;
        default:
            err = -EINVAL;
    }

    if (locked) {
        disk_unlock(disk);
    }

    return err;
}

static off_t ramdisk_lseek(devid_t *dd __unused, off_t offset __unused, int whence __unused) {
    return -ENOSYS;
}

static isize ramdisk_read(devid_t *dd, off_t off, void *buf, usize size) {
    ramdisk_t *disk = get_ramdisk(dd);
    if (disk == NULL || buf == NULL) {
        return -EINVAL;
    }

    bool    locked  = disk_recursive_lock(disk);
    isize   retval  = MIN(disk->size - off, size);
    memcpy(buf, disk->data + off, retval);

    if (locked) {
        disk_unlock(disk);
    }

    return retval;
}

static isize ramdisk_write(devid_t *dd, off_t off, void *buf, usize size) {
    ramdisk_t *disk = get_ramdisk(dd);
    if (disk == NULL || buf == NULL) {
        return -EINVAL;
    }

    bool    locked  = disk_recursive_lock(disk);
    isize   retval  = MIN(disk->size - off, size);
    memcpy(disk->data + off, buf, retval);

    if (locked) {
        disk_unlock(disk);
    }

    return retval;
}
