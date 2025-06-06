#include <bits/errno.h>
#include <dev/dev.h>
#include <dev/pty.h>
#include <lib/printk.h>
#include <string.h>
#include <sys/thread.h>
#include <mm/kalloc.h>

DECL_DEVOPS(static, pts);
static devops_t *pts_devops = DEVOPS_PTR(pts);

int pts_mkslave(PTY pty) {
    int     err     = 0;
    char    name[32]= {0};
    device_t*dev    = NULL;
    mode_t  mode    = S_IFCHR; // mode will be set by grantpt().

    if (pty == NULL)
        return -EINVAL;

    if ((err = device_create(name, FS_CHR, PTS_DEV_MAJOR, pts_devops, &dev)))
        return err;

    if ((err = device_register(dev))) {
        dev_unlock(dev);
        kfree(dev);
        return err;
    }

    dev_unlock(dev);

    memset(name, 0, sizeof name);
    snprintf(name, sizeof name - 1, "/dev/pts/%d", pty->pt_id);

    if ((err = vfs_mknod(name, current_cred(), mode, DEV_T(PTS_DEV_MAJOR, pty->pt_id)))) {
        assert(0, "error mknod: pts, err: %d\n", err);
        dev_unlock(dev);
        kfree(dev);
        return err;
    }

    return 0;
}

__unused static int pts_init(void) {
    return 0;
}

static int pts_probe(struct devid *dd __unused) {
    return 0;
}

static int pts_fini(struct devid *dd __unused) {
    return 0;
}

static int pts_close(struct devid *dd __unused) {
    return 0;
}

static int pts_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOTSUP;
}

static int pts_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int pts_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused) {
    return -ENOTSUP;
}

static off_t pts_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -ENOTSUP;
}

static ssize_t pts_read(struct devid *dd, off_t off __unused, void *buf, size_t sz) {
    int err = 0;
    PTY pty = NULL;

    if (buf == NULL)
        return -EINVAL;

    if ((err = pty_find(dd->minor, &pty)))
        return err;

    ringbuf_lock(pty->master);
    sz = ringbuf_read(pty->master, buf, sz);
    ringbuf_unlock(pty->master);

    pty->pt_refs++;
    pty_unlock(pty);

    return sz;
}

static ssize_t pts_write(struct devid *dd, off_t off __unused, void *buf, size_t sz) {
    int err = 0;
    PTY pty = NULL;

    if (buf == NULL)
        return -EINVAL;

    if ((err = pty_find(dd->minor, &pty)))
        return err;

    ringbuf_lock(pty->slave);
    sz = ringbuf_write(pty->slave, buf, sz);
    ringbuf_unlock(pty->slave);

    pty->pt_refs++;
    pty_unlock(pty);

    return sz;
}

static int pts_mmap(struct devid *dd, vmr_t *region) {
    if (dd == NULL || region == NULL)
        return -EINVAL;
    return -ENOSYS;
}