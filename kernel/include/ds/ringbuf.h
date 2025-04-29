#pragma once

#include <sync/spinlock.h>

#define RINGBUF_INDEX(ring, i) ((i) % ((ring)->size))

typedef struct ringbuf {
    char *buf;
    size_t size;
    size_t head;
    size_t tail;
    size_t count;
    spinlock_t lock;
}ringbuf_t;

#define RINGBUF_NEW(nam, sz) (&(ringbuf_t){.buf = (char[sz]){0}, .size = sz, .head = 0, .tail = 0, .lock = SPINLOCK_INIT()})

#define ringbuf_assert(r)           ({ assert(r, "no ringbuf"); })
#define ringbuf_lock(r)             ({ ringbuf_assert(r); spin_lock(&(r)->lock); })
#define ringbuf_unlock(r)           ({ ringbuf_assert(r); spin_unlock(&(r)->lock); })
#define ringbuf_try_lock(r)         ({ ringbuf_assert(r); spin_try_lock(&(r)->lock); })
#define ringbuf_recursive_lock(r)   ({ ringbuf_assert(r); spin_recursive_lock(&(r)->lock); })
#define ringbuf_assert_locked(r)    ({ ringbuf_assert(r); spin_assert_locked(&(r)->lock); })

extern void ringbuf_debug(ringbuf_t *ring);

extern int ringbuf_new(size_t size, ringbuf_t **);
extern int ringbuf_init(isize size, ringbuf_t *ring);

extern void ringbuf_free_buffer(ringbuf_t *r);
extern void ringbuf_free(ringbuf_t *ring);
extern int ringbuf_isfull(ringbuf_t *ring);
extern int ringbuf_isempty(ringbuf_t *ring);
extern size_t ringbuf_available(ringbuf_t *ring);

extern size_t ringbuf_read(ringbuf_t *ring, char *buf, size_t n);
extern size_t ringbuf_write(ringbuf_t *ring, char *buf, size_t n);