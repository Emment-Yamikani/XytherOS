#pragma once

#include <core/types.h>
#include <dev/rtc.h>
#include <fs/fs.h>
#include <sync/spinlock.h>

/*Character Devices */
enum cdev_major {
    MEM_DEV_MAJOR       = 1,   // minor=1
    NULL_DEV_MAJOR      = 1,   // minor=3
    ZERO_DEV_MAJOR      = 1,   // minor=5
    FULL_DEV_MAJOR      = 1,   // minor=7
    RANDOM_DEV_MAJOR    = 1,   // minor=8
    KBD0_DEV_MAJOR      = 2,   // minor=[0..n-1]
    TTY_DEV_MAJOR       = 4,   // minor=[0..n-1]
    CONSOLE_DEV_MAJOR   = 5,   // minor=1
    PTMX_DEV_MAJOR      = 5,   // minor=2
    UART_DEV_MAJOR      = 6,   // minor=0
    PSAUX_DEV_MAJOR     = 10,  // minor=1
    HPET_DEV_MAJOR      = 10,  // minor=228
    KBDEV_DEV_MAJOR     = 13,  // minor=0
    MOUSE0_DEV_MAJOR    = 13,  // minor=1
    FB_DEV_MAJOR        = 29,  // minor=0
    PTS_DEV_MAJOR       = 136, // minor=[0..n-1]
    CPU_DEV_MAJOR       = 202, // minor=[0..n-1]
    CPU_MSR_DEV_MAJOR   = 203, // minor=[0..n-1]
    RTC0_DEV_MAJOR      = 248, // minor=0
};

/*Block Devices*/
enum bdev_major {
    RAMDISK_DEV_MAJOR   = 1,   // minor=[0..n-1]
    SDA_DEV_MAJOR       = 8,   // minor=[0..n-1]
    CDROM_DEV_MAJOR     = 11,  // minor=[0..n-1]
};

#define DEV_TO_MAJOR(dev)       ((devno_t)((dev) & 0xffu))
#define MAJOR_TO_DEV(major)     ((dev_t)((devno_t)(major) & 0xffu))

#define DEV_TO_MINOR(dev)       ((devno_t)(((dev) >> 8) & 0xffu))
#define MINOR_TO_DEV(minor)     ((dev_t)(((devno_t)(minor) << 8) & 0xff00u))

#define DEV_T(major, minor)     (MINOR_TO_DEV(minor) | MAJOR_TO_DEV(major))

#define DEVID_MAJOR(dd)         ((dd)->major)
#define DEVID_MINOR(dd)         ((dd)->minor)
#define DEVID_TYPE(dd)          ((dd)->type)
#define DEVID_TO_DEVT(dd)       DEV_T(DEVID_MINOR(dd), DEVID_MAJOR(dd))

#define DEVNO_EQ(d0, d1)        ((d0) == (d1))
#define DEVID_EQ(dd0, dd1)      (DEVNO_EQ((dd0)->major, (dd1)->major) && DEVNO_EQ((dd0)->minor, (dd1)->minor))
#define DEVID_CMP(dd0, dd1)     (((dd0) == (dd1) || DEVID_EQ(dd0, dd1)))

#define DEVID(ip, typ, dev) (devid_t){  \
    .inode = (ip),                      \
    .type  = (typ),                     \
    .major = DEV_TO_MAJOR(dev),         \
    .minor = DEV_TO_MINOR(dev),         \
}

#define DEVID_PTR(ip, typ, dev) (&DEVID(ip, typ, dev))

#define DEVID_DEF(_name, _ip, _typ, _dev) devid_t *_name = DEVID_PTR(_ip, _typ, _dev)

#define IDEVID(ip)          DEVID_PTR(ip, INODE_TYPE(ip), INODE_DEV(ip))
#define INODE_TO_DEVID(ip)  DEVID_PTR(ip, INODE_TYPE(ip), INODE_DEV(ip))

#define MAX_MAJOR   256
#define MAX_MINOR   256

static inline bool valid_devno(devno_t num, devno_t limit) {
    return ((num >= 0) && (num < limit));
}

static inline bool valid_major(devno_t major) {
    return valid_devno(major, MAX_MAJOR);
}

static inline bool valid_minor(devno_t minor) {
    return valid_devno(minor, MAX_MINOR);
}

static inline bool valid_device_numbers(devno_t major, devno_t minor) {
    return valid_major(major) && valid_minor(minor);
}

enum {CHRDEV = FS_CHR, BLKDEV = FS_BLK};

static inline bool valid_device_type(int type) {
    return (type == CHRDEV || type == BLKDEV);
}

static inline bool valid_devid(struct devid *dd) {
    return (dd && (valid_device_type(dd->type) && valid_device_numbers(dd->major, dd->minor)));
}

typedef struct devops {
    int     (*probe)(struct devid *dd);
    int     (*fini)(struct devid *dd);
    int     (*close)(struct devid *dd);
    int     (*open)(struct devid *dd, inode_t **pip);
    int     (*getinfo)(struct devid *dd, void *info);
    int     (*mmap)(struct devid *dd, vmr_t *vmregion);
    int     (*ioctl)(struct devid *dd, int request, void *arg);
    off_t   (*lseek)(struct devid *dd, off_t offset, int whence);
    isize   (*read)(struct devid *dd, off_t off, void *buf, usize size);
    isize   (*write)(struct devid *dd, off_t off, void *buf, usize size);
} devops_t;

#define DEVOPS(prefix) (devops_t) {    \
    .probe      = prefix##_probe,      \
    .fini       = prefix##_fini,       \
    .close      = prefix##_close,      \
    .open       = prefix##_open,       \
    .getinfo    = prefix##_getinfo,    \
    .mmap       = prefix##_mmap,       \
    .ioctl      = prefix##_ioctl,      \
    .lseek      = prefix##_lseek,      \
    .read       = prefix##_read,       \
    .write      = prefix##_write,      \
}

#define DEVOPS_PTR(prefix) &DEVOPS(prefix)

#define DECL_DEVOPS(__privacy__, __prefix__)                                             \
__privacy__ int __prefix__##_probe(struct devid *dd);                                    \
__privacy__ int __prefix__##_fini(struct devid *dd);                                     \
__privacy__ int __prefix__##_close(struct devid *dd);                                    \
__privacy__ int __prefix__##_open(struct devid *dd, inode_t **pip);                      \
__privacy__ int __prefix__##_getinfo(struct devid *dd, void *info);                      \
__privacy__ int __prefix__##_mmap(struct devid *dd, vmr_t *vmregion);                    \
__privacy__ int __prefix__##_ioctl(struct devid *dd, int request, void *arg);            \
__privacy__ off_t __prefix__##_lseek(struct devid *dd, off_t offset, int whence);        \
__privacy__ isize __prefix__##_read(struct devid *dd, off_t off, void *buf, usize size); \
__privacy__ isize __prefix__##_write(struct devid *dd, off_t off, void *buf, usize size);

typedef struct device {
    struct devid    devid;
    devops_t        devops;
    atomic_long     refcnt;
    char            name[32];
    void            *private;
    spinlock_t      spinlock;
} device_t;

#define DEVICE_TYPE(dev)    ((dev)->devid.type)

#define DECL_DEVICE(__name, __type, __major, __minor)       \
device_t __name##dev = {                                    \
    .refcnt  = 0,                                           \
    .private = NULL,                                        \
    .name    = #__name,                                     \
    .devops  = DEVOPS(__name),                              \
    .spinlock= SPINLOCK_INIT(),                             \
    .devid   = DEVID(NULL, __type, DEV_T(__major, __minor)),\
}

#define dev_assert(dev)         assert(dev, "Invalid device.")
#define dev_lock(dev)           ({ dev_assert(dev); spin_lock(&(dev)->spinlock); })
#define dev_unlock(dev)         ({ dev_assert(dev); spin_unlock(&(dev)->spinlock); })
#define dev_islocked(dev)       ({ dev_assert(dev); spin_islocked(&(dev)->spinlock); })
#define dev_recursive_lock(dev) ({ dev_assert(dev); spin_recursive_lock(&(dev)->spinlock); })
#define dev_assert_locked(dev)  ({ dev_assert(dev); spin_assert_locked(&(dev)->spinlock); })

static inline int valid_device(device_t *dev) {
    return (dev && valid_devid(&dev->devid)) ? true : false;
}

typedef struct builtin_device {
    void    *arg;
    char    *name;
    int     (*init)();
    int     (*fini)();
} builtin_device_t;

extern builtin_device_t __builtin_devices[];
extern builtin_device_t __builtin_devices_end[];

/**
 * @brief Declare a builtin device driver.
 * 
 * @param n[in] name of device driver.
 * @param i[in] init function of device driver.
 * @param a[in] argument passed to init.
 * @param f[in] fini function of device driver.
 * 
 */
#define BUILTIN_DEVICE(nam, ini, _arg, fin) \
    builtin_device_t __used_section(.__builtin_devices) __dev_##nam = {.name = #nam, .arg = _arg, .init = ini, .fini = fin}

#define foreach_builtin_device() \
    for (builtin_device_t *dev = __builtin_devices; dev < __builtin_devices_end; ++dev)

typedef struct {
    size_t bi_size;
    size_t bi_blocksize;
} bdev_info_t;

extern int  find_bdev_by_name(const char *name, struct devid *dd);
extern int  find_cdev_by_name(const char *name, struct devid *dd);

extern int  device_register(device_t *dev);
extern int  device_unregister(struct devid *dd);

extern void device_destroy(device_t *dev);
extern int  kdevice_create(const char *dev_name, int type, dev_t devid, const devops_t *devops, device_t **pdp);
extern int  device_create(const char *name, int type, devno_t major, const devops_t *devops, device_t **pdp);

extern int      device_probe(struct devid *dd);
extern int      device_fini(struct devid *dd);
extern int      device_close(struct devid *dd);
extern int      device_open(struct devid *dd, inode_t **pip);
extern int      device_getinfo(struct devid *dd, void *info);
extern int      device_mmap(struct devid *dd, vmr_t *vmregion);
extern int      device_ioctl(struct devid *dd, int request, void *arg);
extern off_t    device_lseek(struct devid *dd, off_t offset, int whence);
extern isize    device_read(struct devid *dd, off_t off, void *buf, usize size);
extern isize    device_write(struct devid *dd, off_t off, void *buf, usize size);

extern int dev_init(void);