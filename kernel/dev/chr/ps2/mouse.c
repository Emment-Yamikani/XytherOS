#include <bits/errno.h>
#include <dev/dev.h>
#include <dev/ps2.h>

DECL_DEVOPS(static, ps2mouse);

static DECL_DEVICE(ps2mouse, FS_CHR, MOUSE0_DEV_MAJOR, 1);

int ps2mouse_init(void) {
    return device_register(&ps2mousedev);
}

static int ps2mouse_probe(struct devid *dd __unused) {
    return 0;
}

static int ps2mouse_fini(struct devid *dd __unused) {
    return 0;
}

static int ps2mouse_close(struct devid *dd __unused) {
    return 0;
}

static int ps2mouse_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOTSUP;
}

static int ps2mouse_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int ps2mouse_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused) {
    return -ENOTSUP;
}

static off_t ps2mouse_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -ENOTSUP;
}

static ssize_t ps2mouse_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz __unused) {
    return -ENOTSUP;
}

static ssize_t ps2mouse_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz __unused) {
    return -ENOTSUP;
}

static int ps2mouse_mmap(struct devid *dd, vmr_t *region) {
    if (dd == NULL || region == NULL)
        return -EINVAL;
    return -ENOTSUP;
}