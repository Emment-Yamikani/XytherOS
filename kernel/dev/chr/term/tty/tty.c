#include <bits/errno.h>
#include <dev/tty.h>
#include <lib/printk.h>
#include <string.h>
#include <sys/thread.h>

DECL_DEVOPS(static, tty);

static tty_t ttys[NTTY];

static int tty_init(void) {
    device_t *dev = NULL;
    char     tty_name[8];
    
    memset(ttys, 0, sizeof ttys);
    
    for (int tty = 0; tty < NTTY; ++tty) {
        snprintf(tty_name, sizeof tty_name - 1, "tty%d", tty);
        
        int err = dev_create(tty_name, CHRDEV, DEV_TTY, DEVOPS_PTR(tty), &dev);
        if (err != 0) {
            perror("Failed to create tty device descriptor", err);
            return err;
        }

        ttys[tty].dev   = dev;

        printk("Initializing \e[025453;011m%s\e[0m chardev...\n", dev->name);

        if ((err = dev_register(dev))) {
            for (int tty = 0; tty < NTTY; ++tty) {
                int err = 0;
    
                if ((err = dev_unregister(DEVID_PTR(NULL, FS_CHR, DEV_T(DEV_TTY, tty))))) {
                    printk("%s:%d: Error unregistering device, err: %d\n",
                        __FILE__, __LINE__, err);

                    // FIXME: is continuing the bes thing to do here or freeing dev?
                    continue;
                }
                dev_destroy(ttys[tty].dev);
            }

            printk("Failed to create tty: %s\n", strerror(err));
            return err;
        }
        dev_unlock(dev); // dev was locked in dev_create();
    }

    return 0;
} BUILTIN_DEVICE(tty, tty_init, NULL, NULL);

static int tty_probe(struct devid *dd __unused) {
    return 0;
}

static int tty_fini(struct devid *dd __unused) {
    return 0;
}

static int tty_close(struct devid *dd __unused) {
    return 0;
}

static int tty_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int tty_mmap(struct devid *dd __unused, vmr_t *region __unused) {
    return -ENOSYS;
}

static int tty_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOSYS;
}

static off_t tty_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -ENOSYS;
}

static int tty_ioctl(struct devid *dd __unused, int request __unused, void *arg __unused) {
    return -ENOSYS;
}

static ssize_t tty_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t nbyte __unused) {
    return 0;
}

static ssize_t tty_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t nbyte __unused) {
    return 0;
}