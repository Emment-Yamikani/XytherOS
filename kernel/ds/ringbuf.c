#include <ds/ringbuf.h>
#include <bits/errno.h>
#include <core/types.h>
#include <mm/kalloc.h>

int ringbuf_init(isize size, ringbuf_t *ring) {
    u8 *buf = NULL;

    if (ring == NULL)
        return -EINVAL;

    if (!(buf = (u8 *)kmalloc(size))) {
        kfree(ring);
        return -ENOMEM;
    }

    *ring = (ringbuf_t) {
        .head = 0,
        .tail = 0,
        .size = size,
        .buf  = (char *)buf,
        .lock = SPINLOCK_INIT(),
    };

    return 0;
}

int ringbuf_new(size_t size, ringbuf_t **rref) {
    int       err   = 0;
    ringbuf_t *ring = NULL;

    assert(rref, "no reference");

    if ((ring = kmalloc(sizeof(ringbuf_t))) == NULL)
        return -ENOMEM;

    if ((err = ringbuf_init(size, ring)))
        goto error;

    *rref = ring;
    return 0;
error:
    if (ring != NULL)
        kfree(ring);
    return err;
}

void ringbuf_free_buffer(ringbuf_t *r) {
    ringbuf_assert(r);
    if (r->buf) {
        kfree(r->buf);
        r->head = 0;
        r->size = 0;
        r->tail = 0;
        r->count= 0;
        r->buf  = NULL;
    }
}

void ringbuf_free(ringbuf_t *r) {
    assert(r, "no ringbuff");
    kfree(r->buf);
    kfree(r);
}

int ringbuf_isempty(ringbuf_t *ring) {
    ringbuf_assert_locked(ring);
    return ring->count == 0;
}

int ringbuf_isfull(ringbuf_t *ring) {
    ringbuf_assert_locked(ring);
    return ring->count == ring->size;
}

size_t ringbuf_read(ringbuf_t *ring, char *buf, size_t n) {
    size_t size = n;
    ringbuf_assert_locked(ring);
    assert(buf, "No buffer provided for the rinfbuffer function.");

    while (n) {
        if (ringbuf_isempty(ring))
            break;
        *buf++ = ring->buf[RINGBUF_INDEX(ring, ring->head++)];
        ring->count--;
        n--;
    }
    return size - n;
}

size_t ringbuf_write(ringbuf_t *ring, char *buf, size_t n) {
    size_t size = n;
    ringbuf_assert_locked(ring);
    assert(buf, "No buffer provided for the rinfbuffer function.");

    while (n) {
        if (ringbuf_isfull(ring))
            break;

        ring->buf[RINGBUF_INDEX(ring, ring->tail++)] = *buf++;
        ring->count++;
        n--;
    }
    return size - n;
}

size_t ringbuf_available(ringbuf_t *ring) {
    ringbuf_assert_locked(ring);
    if (ring->tail >= ring->head)
        return ring->tail - ring->head;
    size_t aval = ring->tail + ring->size - ring->head;
    return aval;
}

void ringbuf_debug(ringbuf_t *ring) {
    ringbuf_lock(ring);
    printk("size: %5d\nhead: %5d\ntail: %5d\ncount: %d\n",
        ring->size, RINGBUF_INDEX(ring, ring->head), RINGBUF_INDEX(ring, ring->tail), ring->count);
    ringbuf_unlock(ring);
}