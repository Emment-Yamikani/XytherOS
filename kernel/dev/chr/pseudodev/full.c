#include <bits/errno.h>
#include <dev/dev.h>
#include <lib/printk.h>
#include <string.h>
#include <sync/spinlock.h>

DECL_DEVOPS(static, full);

static DECL_DEVICE(full, FS_CHR, DEV_FULL, 7);

static int full_init(void) {
    return device_register(&fulldev);
}

static int full_probe(struct devid *dd __unused) {
    return 0;
}

static int full_fini(struct devid *dd __unused) {
    return 0;
}

static int full_close(struct devid *dd __unused) {
    return 0;
}

static int full_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOTSUP;
}

static int full_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int full_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused) {
    return 0;
}

static off_t full_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return 0;
}

static ssize_t full_read(struct devid *dd __unused, off_t off __unused, void *buf, size_t sz) {
    if (buf == NULL)
        return -EINVAL;
    memset(buf, 0, sz);
    return sz;
}

static ssize_t full_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz __unused) {
    return -ENOSPC;
}

static int full_mmap(struct devid *dd, vmr_t *region) {
    if (dd == NULL || region == NULL)
        return -EINVAL;
    
    return -ENOSYS;
}

BUILTIN_DEVICE(full, full_init, NULL, NULL);