#pragma once

#include <ds/queue.h>
#include <sync/atomic.h>
#include <sync/spinlock.h>

typedef struct mtx_t {
    atomic_u8   m_locked;
    atomic_u64  m_recurs;
    thread_t    *m_owner;
    queue_t     m_waitQ;
    spinlock_t  m_guard;
} mtx_t;

#define MTX_INLINE_INIT(mtx_name)                        \
    (mtx_t)                                              \
    {                                                    \
        .m_guard = SPINLOCK_INIT(),                      \
        .m_locked = false,                               \
        .m_recurs = 0,                                   \
        .m_owner = NULL,                                 \
        .m_waitQ = QUEUE_INLINE_INIT((mtx_name).m_waitQ) \
    }

#define MTX(mtx_name) mtx_t mtx_name = MTX_INLINE_INIT(mtx_name)

#define mtx_assert(mtx) ({ assert(mtx, "Invalid mtx.\n"); })


extern int      mtx_init(mtx_t *mtx);
extern void     mtx_lock(mtx_t *mtx);
extern void     mtx_unlock(mtx_t *mtx);
extern int      mtx_trylock(mtx_t *mtx);
extern int      mtx_islocked(mtx_t *mtx);
extern void     mtx_assert_locked(mtx_t *mtx);
extern void     mtx_recursive_lock(mtx_t *mtx);