#include <bits/errno.h>
#include <core/debug.h>
#include <dev/dev.h>
#include <dev/tty.h>
#include <dev/kbd_event.h>
#include <ds/ringbuf.h>
#include <ds/queue.h>
#include <sys/schedule.h>
#include <sys/thread.h>

#define KBD_BUFFSZ (16834 * sizeof(kbd_event_t)) // size of keyboard buffer.

static QUEUE(event_readers);
static QUEUE(event_writers);
static ringbuf_t    event_buffer;
static thread_t     *grabber = NULL;
DECL_DEVOPS(static, event0);

static DECL_DEVICE(event0, CHRDEV, KBDEV_DEV_MAJOR, 0);

static int event0_init(void) {
    int err = ringbuf_init(KBD_BUFFSZ, &event_buffer);
    if (err) {
        return err;
    }

    if ((err = queue_init(event_writers))) {
        return err;
    }

    if ((err = queue_init(event_readers))) {
        return err;
    }

    return device_register(&event0dev);
} BUILTIN_DEVICE(event, event0_init, NULL, NULL);

int kbd_get_event(kbd_event_t *ev) {
    int err;

    if (ev == NULL) {
        return -EINVAL;
    }

    ringbuf_lock(&event_buffer);

    loop() {
        err = ringbuf_read(&event_buffer, (void *)ev, sizeof(kbd_event_t));

        sched_wakeup_all(event_writers, WAKEUP_NORMAL, NULL);
        if (err == 0) {
            if (current == NULL) {
                continue;
            }

            err = sched_wait_whence(event_readers, T_SLEEP, QUEUE_TAIL, NULL, &event_buffer.lock);
            if (err == 0) {
                continue;
            }
        }

        if (err < 0) {
            break;
        } else if (err == sizeof(kbd_event_t)) {
            err = 0;
            break;
        } else {
            err = -EFAULT;
            break;
        }
    }

    ringbuf_unlock(&event_buffer);
    return err;
}

int async_kbd_inject_event(kbd_event_t *ev) {
    if (ev == NULL) {
        return -EINVAL;
    }

    tty_t *tp = tty_current();
    int err = tty_receive_input(tp, ev);
    if (err == 0 || err != -EIO) {
        return err;
    }

    ringbuf_lock(&event_buffer);
    err = ringbuf_write(&event_buffer, (void *)ev, sizeof(kbd_event_t));
    sched_wakeup_all(event_readers, WAKEUP_NORMAL, NULL);

    err = (err == sizeof (kbd_event_t)) ? 0 : err < 0 ? err : -EFAULT;
    ringbuf_unlock(&event_buffer);
    return err;
}

int kbd_inject_event(kbd_event_t *ev) {
    int err;

    if (ev == NULL) {
        return -EINVAL;
    }

    ringbuf_lock(&event_buffer);

    loop() {
        err = ringbuf_write(&event_buffer, (void *)ev, sizeof(kbd_event_t));

        sched_wakeup_all(event_readers, WAKEUP_NORMAL, NULL);
        if (err == 0) {
            if (current == NULL) {
                continue;
            }

            err = sched_wait_whence(event_writers, T_SLEEP, QUEUE_TAIL, NULL, &event_buffer.lock);
            if (err == 0) {
                continue;
            }
        }

        if (err < 0) {
            break;
        } else if (err == sizeof(kbd_event_t)) {
            err = 0;
            break;
        } else {
            err = -EFAULT;
            break;
        }
    }

    ringbuf_unlock(&event_buffer);

    return err;
}

static int event0_probe(struct devid *) {
    return 0;
}

static int event0_fini(struct devid *) {
    return 0;
}

static int event0_close(struct devid *) {
    return 0;
}

static int event0_open(struct devid *, inode_t **pip __unused) {
    thread_t *thread = atomic_load(&grabber);
    if (thread && thread != current) {
        return -EBUSY;
    }

    atomic_store(&grabber, current);
    return 0;
}

static int event0_getinfo(struct devid *, void *info __unused) {
    return -ENOSYS;
}

static int event0_mmap(struct devid *, vmr_t *vmregion __unused) {
    return -EOPNOTSUPP;
}

static int event0_ioctl(struct devid *, int request, void *arg) {
    switch (request) {
        case EV_GRAB:
            if (atomic_load(&grabber)) {
                return -EBUSY;
            }

            atomic_store(&grabber, current);
            return 0;
        case EV_UNGRAB:
            if (atomic_load(&grabber) != current) {
                return -EPERM;
            }

            atomic_store(&grabber, NULL);
            return 0;
        case EV_READ:
            thread_t *thread = atomic_load(&grabber);
            if (thread && thread != current) {
                return -EBUSY;
            }

            return kbd_get_event(arg);
        case EV_WRITE:

            return kbd_inject_event(arg);
        default:
            return -EINVAL;
    }
}

static off_t event0_lseek(struct devid *, off_t offset __unused, int whence __unused) {
    return -EOPNOTSUPP;
}

static isize event0_read(struct devid *, off_t, void *buf, usize size) {
    if (!buf || size < sizeof(kbd_event_t)) {
        return -EINVAL;
    }

    usize       events_read = 0;
    kbd_event_t *evbuf      = (kbd_event_t *)buf;
    usize       count       = size / sizeof(kbd_event_t);

    for (; events_read < count; ++events_read) {
        int err = kbd_get_event(&evbuf[events_read]);
        if (err < 0) {
            if (events_read > 0) {
                // Return how many we read before error
                return events_read * sizeof(kbd_event_t);
            }

            return err;
        }
    }

    return events_read * sizeof(kbd_event_t);
}

static isize event0_write(struct devid *, off_t, void *buf, usize size) {
    if (!buf || size < sizeof(kbd_event_t)) {
        return -EINVAL;
    }

    usize       events_written  = 0;
    kbd_event_t *evbuf          = (kbd_event_t *)buf;
    usize       count           = size / sizeof(kbd_event_t);

    for (; events_written < count; ++events_written) {
        int err = kbd_inject_event(&evbuf[events_written]);
        if (err < 0) {
            if (events_written > 0) {
                return events_written * sizeof(kbd_event_t);
            }

            return err;
        }
    }

    return events_written * sizeof(kbd_event_t);
}
