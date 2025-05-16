#include <bits/errno.h>
#include <dev/console.h>
#include <dev/tty.h>
#include <lib/printk.h>
#include <mm/kalloc.h>
#include <string.h>
#include <sys/thread.h>

extern devops_t ttydev_ops;

DECL_DEVOPS(static, tty);

devops_t ttydev_ops = DEVOPS(tty);

tty_t   *active_tty   = NULL;
ldisc_t *active_ldisc = NULL;

static tty_t *ttys[NTTY];

int tty_alloc_buffer(tty_t *tp) {
    tty_assert(tp);

    int err;
    if ((err = tty_inputbuf_init(tp))) {
        return err;
    }

    return 0;
}

void tty_free_buffers(tty_t *tp) {
    tty_assert(tp);

    ringbuf_free_buffer(&tp->t_inputbuf);
}

int tty_alloc(tty_t **ptp) {
    if (ptp == NULL) {
        return -EINVAL;
    }

    tty_t *tp = kzalloc(sizeof *tp);
    if (tp == NULL) {
        return -ENOMEM;
    }

    tty_init_lock(tp);

    int err = await_event_init(&tp->t_readers);
    if (err) {
        return err;
    }

    err = await_event_init(&tp->t_writers);
    if (err) {
        return err;
    }

    err = tty_alloc_buffer(tp);
    if (err) {
        return err;
    }

    *ptp = tp;
    return 0;
}

void tty_free(tty_t *tp) {
    tty_assert(tp);

    tty_free_buffers(tp);

    if (tp->t_dev) {
        device_destroy(tp->t_dev);
    }

    kfree(tp);
}

/**
 * tty_init_termios - Initialize a termios struct with standard defaults
 * @tio: termios structure to initialize
 * 
 * Sets up common default values for terminal I/O:
 * - Raw input mode (non-canonical)
 * - Echo enabled
 * - Standard control characters (Ctrl-C for interrupt, etc.)
 * - 8-bit characters, no parity
 */
void tty_init_termios(struct termios *tio) {
    /* Control modes */
    tio->c_cflag = CS8 | CREAD | CLOCAL;  /* 8n1, enable receiver */
    tio->c_iflag = ICRNL | IXON;          /* CR->NL, enable flow control */
    tio->c_oflag = OPOST | ONLCR;         /* Post-process output, NL->CR-NL */
    tio->c_lflag = ISIG | ICANON | ECHO | ECHONL | ECHOE | ECHOK | IEXTEN;

    /* Control characters */
    tio->c_cc[VINTR]    = CTRL('C');     /* Ctrl-C */
    tio->c_cc[VQUIT]    = CTRL('\\');    /* Ctrl-\ */
    tio->c_cc[VERASE]   = 0x7F;          /* DEL */
    tio->c_cc[VKILL]    = CTRL('U');     /* Ctrl-U */
    tio->c_cc[VEOF]     = CTRL('D');     /* Ctrl-D */
    tio->c_cc[VTIME]    = 0;             /* No timeout */
    tio->c_cc[VMIN]     = 1;             /* Read 1 char at a time */
    tio->c_cc[VSTART]   = CTRL('Q');     /* Ctrl-Q */
    tio->c_cc[VSTOP]    = CTRL('S');     /* Ctrl-S */
    tio->c_cc[VSUSP]    = CTRL('Z');     /* Ctrl-Z */
    tio->c_cc[VLNEXT]   = CTRL('V');     /* Ctrl-V */
    tio->c_cc[VWERASE]  = CTRL('W');     /* Ctrl-W */
    tio->c_cc[VREPRINT] = CTRL('R');     /* Ctrl-R */
    tio->c_cc[VDISCARD] = CTRL('O');     /* Ctrl-O */
#ifdef _POSIX_VDISABLE
    tio->c_cc[VEOL]     = _POSIX_VDISABLE; /* Disable extra EOL */
    tio->c_cc[VEOL2]    = _POSIX_VDISABLE; /* Disable extra EOL */
#endif

    /* Baud rate - typically B38400 or B115200 for modern systems */
    tio->c_ispeed = tio->c_ospeed = B38400;
}

int tty_create(const char *name, devno_t minor, tty_ops_t *ops, ldisc_t *ldisc, tty_t **ptp) {
    if (!name || !ptp) {
        return -EINVAL;
    }

    if (minor != 0 && ops == NULL) {
        return -ENOSYS; // need to provide tty operations.
    }

    tty_t *tp;
    int err;
    if ((err = tty_alloc(&tp))) {
        return err;
    }

    dev_t dev = DEV_T(TTY_DEV_MAJOR, minor);

    err = kdevice_create(name, CHRDEV, dev, &ttydev_ops, &tp->t_dev);
    if (err) {
        tty_free(tp);
        return err;
    }

    tty_init_termios(&tp->t_termios);

    tp->t_tabsize = 8;
    tp->t_dev->private = tp;
    tp->t_ops   = ops ? *ops : (tty_ops_t){0};
    tp->t_ldisc = ldisc ? ldisc : &ldisc_X_TTY;

    *ptp = tp;
    return 0;
}

static int tty_init(void) {

    memset(ttys, 0, sizeof ttys);

    int err = ldisc_init();
    if (err) {
        return err;
    }

    tty_t *tty;
    err = tty_create("tty0", 0, NULL, NULL, &tty);
    if (err)  return err;

    err = device_register(tty->t_dev);
    if (err) return err;

    err = console_init();
    if (err) return err;

    active_tty = ttys[1] = console;

    return 0;
} BUILTIN_DEVICE(tty, tty_init, NULL, NULL);

static int get_tty(devid_t *dd, tty_t **ptp) {
    if (ptp == NULL) {
        return -EINVAL;
    }

    if (!valid_devid(dd)) {
        return -ENXIO;
    }

    if (dd->major != TTY_DEV_MAJOR) {
        return -ENOTTY;
    }

    tty_t *tp = dd->minor == 0 ? atomic_read(&active_tty) : ttys[dd->minor];

    if (tp == NULL) {
        return -ENXIO;
    }

    tty_lock(tp);
    *ptp = tp;
    return 0;
}

static int tty_probe(struct devid *dd __unused) {
    return 0;
}

static int tty_fini(struct devid *dd __unused) {
    return 0;
}

static int tty_close(struct devid *dd) {
    tty_t *tp;
    int err = get_tty(dd, &tp);

    if (err != 0) {
        return err;
    }

    if (tp->t_ops.close == NULL) {
        tty_unlock(tp);
        return -ENOSYS;
    }

    tp->t_ops.close(tp);
    tty_unlock(tp);
    return 0;
}

static int tty_open(struct devid *dd, inode_t **) {
    tty_t *tp;
    int err = get_tty(dd, &tp);
    if (err != 0) {
        return err;
    }

    if (tp->t_ops.open == NULL) {
        tty_unlock(tp);
        return -ENOSYS;
    }

    tp->t_flags |= T_OPEN;

    tp->t_ops.open(tp);
    tty_unlock(tp);
    return 0;
}

static int tty_mmap(struct devid *, vmr_t *) {
    return -ENOSYS;
}

static int tty_getinfo(struct devid *, void *) {
    return -ENOSYS;
}

static off_t tty_lseek(struct devid *, off_t, int) {
    return -ENOSYS;
}

static int tty_ioctl(struct devid *dd, int request, void *arg) {
    tty_t *tp;
    int err = get_tty(dd, &tp);

    if (err != 0) {
        return err;
    }

    if (tp->t_ops.ioctl == NULL) {
        tty_unlock(tp);
        return -ENOSYS;
    }

    tp->t_ops.ioctl(tp, request, arg);
    tty_unlock(tp);
    return -ENOSYS;
}

static ssize_t tty_read(struct devid *dd, off_t, void *buf, size_t nbyte) {
    tty_t *tp;
    ssize_t ret = get_tty(dd, &tp);

    if (ret != 0) {
        return ret;
    }

    if (!(tp->t_flags & T_OPEN)) {
        tty_unlock(tp);
        return -ENOTTY;
    }

    if (!tp->t_ldisc || !tp->t_ldisc->read) {
        tty_unlock(tp);
        return -ENOSYS;
    }

    ret = tp->t_ldisc->read(tp, buf, nbyte);
    tty_unlock(tp);
    return ret;
}

static ssize_t tty_write(struct devid *dd, off_t, void *buf, size_t nbyte) {
    tty_t *tp;
    ssize_t ret = get_tty(dd, &tp);

    if (ret != 0) {
        return ret;
    }

    if (!(tp->t_flags & T_OPEN)) {
        tty_unlock(tp);
        return -ENOTTY;
    }

    if (!tp->t_ldisc || !tp->t_ldisc->write) {
        tty_unlock(tp);
        return -ENOSYS;
    }

    ret = tp->t_ldisc->write(tp, buf, nbyte);
    tty_unlock(tp);
    return ret;
}