#include <bits/errno.h>
#include <dev/dev.h>
#include <lib/printk.h>
#include <string.h>
#include <sync/spinlock.h>

DECL_DEVOPS(static, null);
static DECL_DEVICE(null, FS_CHR, NULL_DEV_MAJOR, 3);

static int null_init(void) {
    return device_register(&nulldev);
}

static int null_probe(struct devid *dd __unused) {
    return 0;
}

static int null_fini(struct devid *dd __unused) {
    return 0;
}

static int null_close(struct devid *dd __unused) {
    return 0;
}

static int null_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOTSUP;
}

static int null_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int null_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused) {
    return -ENOTSUP;
}

static off_t null_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -ENOTSUP;
}

static ssize_t null_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz __unused) {
    return 0;
}

static ssize_t null_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz) {
    return sz;
}

static int null_mmap(struct devid *dd, vmr_t *region) {
    if (dd == NULL || region == NULL)
        return -EINVAL;
    
    return -ENOSYS;
}

BUILTIN_DEVICE(null, null_init, NULL, NULL);