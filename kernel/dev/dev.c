#include <dev/dev.h>
#include <ds/bitmap.h>
#include <mm/kalloc.h>
#include <string.h>
#include <sys/thread.h>

typedef struct {
    bitmap_t    minor_map[MAX_MAJOR];
    device_t    *devices[MAX_MAJOR][MAX_MINOR];
    spinlock_t  spinlock;
} device_table_t;

static device_table_t *bdev = &(device_table_t){0};
static device_table_t *cdev = &(device_table_t){0};

#define table_assert(t)         assert(t, "Invalid device table.")
#define table_lock(t)           ({ table_assert(t); spin_lock(&(t)->spinlock); })
#define table_unlock(t)         ({ table_assert(t); spin_unlock(&(t)->spinlock); })
#define table_islocked(t)       ({ table_assert(t); spin_islocked(&(t)->spinlock); })
#define table_assert_locked(t)  ({ table_assert(t); spin_assert_locked(&(t)->spinlock); })

#define table_peek_device(t, major, minor) ({ table_assert_locked(t); (t)->devices[major][minor]; })

static inline device_table_t *get_device_table(int type) {
    if (!valid_device_type(type)) {
        return NULL;
    }
    
    device_table_t *table;
    table = (device_table_t *[]){
        [CHRDEV] = cdev,
        [BLKDEV] = bdev,
    }[type];

    table_lock(table);
    return table;
}

static int dev_alloc_minor(int type, devno_t major, devno_t *pminor) {
    if (!pminor || !valid_device_type(type) || !valid_major(major)) {
        return -EINVAL;
    }

    devno_t minor = 0;
    device_table_t *table = get_device_table(type);

    int err = bitmap_alloc_range(&table->minor_map[major], 1, (usize *)&minor);

    table_unlock(table);

    if (err == 0) {
        *pminor = minor;
    }

    return err;
}

int dev_alloc_minor_set(int type, devno_t major, int pos, int cnt) {
    if (!valid_mode_type(type) || !valid_major(major)) {
        return -EINVAL;
    }

    if (!valid_minor(pos) || (cnt + pos) >= MAX_MINOR) {
        return -EINVAL;
    }

    device_table_t *table = get_device_table(type);
    int err = bitmap_set(&table->minor_map[major], pos, cnt);
    table_unlock(table);

    return err;
}

static device_t *get_device_by_devid(struct devid *dd) {
    if (!valid_devid(dd)) {
        return NULL;
    }
    
    device_table_t *table = get_device_table(dd->type);
    device_t *dev = table_peek_device(table, dd->major, dd->minor);

    if (dev) {
        atomic_inc(&dev->refcnt);
    }

    table_unlock(table);
    return dev;
}

static int get_device_by_name(const char *name, int type, struct devid *dd) {
    if (!name || !valid_device_type(type) || !dd) {
        return -EINVAL;
    }

    device_table_t *table = get_device_table(type);

    for (usize major = 0; major < MAX_MAJOR; ++major) {
        for (usize minor = 0; minor < MAX_MINOR; ++minor) {
            device_t *dev = table->devices[major][minor];
            if (dev && string_eq(name, dev->name)) {
                *dd = dev->devid;
                table_unlock(table);
                return 0;
            }
        }
    }

    table_unlock(table);
    return -ENODEV;
}

static inline device_t *device_get(int type, devno_t major, devno_t minor) {
    if (!valid_device_type(type) || !valid_device_numbers(major, minor)) {
        return NULL;
    }

    device_table_t *table = get_device_table(type);
    device_t *dev = table_peek_device(table, major, minor);

    if (dev) {
        atomic_inc(&dev->refcnt);
    }

    table_unlock(table);
    return dev;
}

static inline int device_already_registered(int type, dev_t devid) {
    devno_t major = DEV_TO_MAJOR(devid), minor = DEV_TO_MINOR(devid);

    if (!valid_device_type(type) || !valid_device_numbers(major, minor)) {
        return -EINVAL;
    }

    device_table_t *table = get_device_table(type);
    device_t *dev = table_peek_device(table, major, minor);

    table_unlock(table);

    return dev ? 0 : -ENODEV;
}

static inline device_t *get_bdev(devno_t major, devno_t minor) {
    return device_get(BLKDEV, major, minor);
}

static inline device_t *get_cdev(devno_t major, devno_t minor) {
    return device_get(CHRDEV, major, minor);
}

/**
 * @brief Initialize device multiplexer
 * 
 * @return int 
 */
int device_mux_init(void) {
    for (usize major = 0; major < MAX_MAJOR; ++major) {
        for (usize minor = 0; minor < MAX_MINOR; ++minor) {
            bdev->devices[major][minor] = NULL;
            cdev->devices[major][minor] = NULL;
        }

        int err = bitmap_init_array(&bdev->minor_map[major], MAX_MINOR);
        if (err != 0) {
            return err;
        }

        err = bitmap_init_array(&cdev->minor_map[major], MAX_MINOR);
        if (err != 0) {
            kfree(bdev->minor_map[major].bm_map);
            return err;
        }
    }

    spinlock_init(&bdev->spinlock);
    spinlock_init(&cdev->spinlock);

    printk("Device multiplexer initialized.\n");
    return 0;
}

int find_bdev_by_name(const char *name, struct devid *dd) {
    return get_device_by_name(name, BLKDEV, dd);
}

int find_cdev_by_name(const char *name, struct devid *dd) {
    return get_device_by_name(name, CHRDEV, dd);
}

int kdev_create(const char *dev_name, int type, dev_t devid, const devops_t *devops, device_t **pdp) {
    if (!dev_name) {
        return -EINVAL;
    }

    const usize name_len = strlen(dev_name);
    if (name_len >= sizeof ((device_t){0}).name) {
        return -ENAMETOOLONG;
    }

    device_t *dev = kmalloc(sizeof (device_t));
    if (dev == NULL) {
        return -ENOMEM;
    }

    memset(dev, 0, sizeof *dev);

    int err = device_already_registered(type, devid);
    if (err != -ENODEV) {
        kfree(dev);
        err = err == 0 ? -EEXIST : err;
        return err;
    }

    devno_t major = DEV_TO_MAJOR(devid), minor = DEV_TO_MINOR(devid);
    device_table_t *table = get_device_table(type);
    bitmap_set(&table->minor_map[major], minor, 1);
    table_unlock(table);

    dev->private= NULL;
    dev->devid  = DEVID(NULL, type, devid);
    dev->devops = devops ? *devops : (devops_t){0};

    spinlock_init(&dev->spinlock);
    safestrncpy(dev->name, dev_name, sizeof (dev->name) - 1);

    dev_lock(dev);

    *pdp = dev;
    return 0;
}

int dev_create(const char *name, int type, devno_t major, const devops_t *devops, device_t **pdp) {
    devno_t minor = 0;
    
    int err = dev_alloc_minor(type, major, &minor);
    if (err != 0) {
        return err;
    }
    
    err = kdev_create(name, type, DEV_T(major, minor), devops, pdp);
    if (err != 0) {
        device_table_t *table = get_device_table(type);
        bitmap_unset(&table->minor_map[major], minor, 1);
        table_unlock(table);
        return err;
    }

    return err;
}

void dev_destroy(device_t *dev) {
    if (dev == NULL) {
        return;
    }

    dev_recursive_lock(dev);

    if (atomic_dec(&dev->refcnt) <= 0) {
        dev_unlock(dev);
        return;
    }

    dev_unlock(dev);

    kfree(dev);
}

int dev_register(device_t *dev) {
    devno_t major, minor;

    if (!valid_device(dev)) {
        return -EINVAL;
    }

    major = dev->devid.major;
    minor = dev->devid.minor;
    
    if (dev->devops.probe) {
        int err = dev->devops.probe(&dev->devid);
        if (err != 0) {
            return err;
        }
    }

    device_table_t *table = get_device_table(dev->devid.type);
    
    if (table_peek_device(table, major, minor)) {
        table_unlock(table);
        return -EEXIST;
    }
    
    table->devices[major][minor] = dev;
    atomic_inc(&dev->refcnt);

    bitmap_set(&table->minor_map[major], minor, 1);

    table_unlock(table);

    printk("Registered %s[\e[033m%s\e[0m]...\n", DEVICE_TYPE(dev) == CHRDEV ? "chardev" : "blkdev", dev->name);

    return 0;
}

int dev_unregister(struct devid *dd) {
    device_t       *dev;
    device_table_t *table;

    if (!valid_devid(dd)) {
        return -EINVAL;
    }

    table = get_device_table(dd->type);
    dev   = table_peek_device(table, dd->major, dd->minor);

    if (dev == NULL) {
        table_unlock(table);
        return -ENODEV;
    }

    table->devices[dd->major][dd->minor] = NULL;
    atomic_dec(&dev->refcnt);

    table_unlock(table);

    if (dev->devops.fini) {
        return dev->devops.fini(&dev->devid);
    }

    return 0;
}

int dev_probe(struct devid *dd) {
    device_t *dev = get_device_by_devid(dd);
    if (dev == NULL) {
        return -ENXIO;
    }

    if (dev->devops.probe == NULL) {
        return -ENOSYS;
    }

    return dev->devops.probe(dd);
}

int dev_fini(struct devid *dd) {
    device_t *dev = get_device_by_devid(dd);
    if (dev == NULL) {
        return -ENXIO;
    }

    if (dev->devops.fini == NULL) {
        return -ENOSYS;
    }

    return dev->devops.fini(dd);
}

int dev_close(struct devid *dd) {
    device_t *dev = get_device_by_devid(dd);
    if (dev == NULL) {
        return -ENXIO;
    }

    if (dev->devops.close == NULL) {
        return -ENOSYS;
    }

    return dev->devops.close(dd);
}

int dev_open(struct devid *dd, inode_t **pip) {
    device_t *dev = get_device_by_devid(dd);
    if (dev == NULL) {
        return -ENXIO;
    }

    if (dev->devops.open == NULL) {
        return -ENOSYS;
    }

    return dev->devops.open(dd, pip);
}

int dev_getinfo(struct devid *dd, void *info) {
    device_t *dev = get_device_by_devid(dd);
    if (dev == NULL) {
        return -ENXIO;
    }

    if (info == NULL) {
        return -EFAULT;
    }

    if (dev->devops.getinfo == NULL) {
        return -ENOSYS;
    }

    return dev->devops.getinfo(dd, info);
}

int dev_mmap(struct devid *dd, vmr_t *vmregion) {
    device_t *dev = get_device_by_devid(dd);
    if (dev == NULL) {
        return -ENXIO;
    }

    if (vmregion == NULL) {
        return -EFAULT;
    }

    if (dev->devops.mmap == NULL) {
        return -ENOSYS;
    }

    return dev->devops.mmap(dd, vmregion);
}

int dev_ioctl(struct devid *dd, int request, void *arg) {
    device_t *dev = get_device_by_devid(dd);
    if (dev == NULL) {
        return -ENXIO;
    }

    if (dev->devops.ioctl == NULL) {
        return -ENOSYS;
    }

    return dev->devops.ioctl(dd, request, arg);
}

off_t dev_lseek(struct devid *dd, off_t offset, int whence) {
    device_t *dev = get_device_by_devid(dd);
    if (dev == NULL) {
        return -ENXIO;
    }

    if (dev->devops.lseek == NULL) {
        return -ENOSYS;
    }

    return dev->devops.lseek(dd, offset, whence);
}

isize dev_read(struct devid *dd, off_t off, void *buf, usize size) {
    device_t *dev = get_device_by_devid(dd);
    if (dev == NULL) {
        return -ENXIO;
    }

    if (dev->devops.read == NULL) {
        return -ENOSYS;
    }

    return dev->devops.read(dd, off, buf, size);
}

isize dev_write(struct devid *dd, off_t off, void *buf, usize size) {
    device_t *dev = get_device_by_devid(dd);
    if (dev == NULL) {
        return -ENXIO;
    }

    if (dev->devops.write == NULL) {
        return -ENOSYS;
    }

    return dev->devops.write(dd, off, buf, size);
}
