#include <bits/errno.h>
#include <sync/mutex.h>
#include <sys/schedule.h>
#include <sys/thread.h>

int mtx_init(mtx_t *mtx) {
    if (mtx == NULL)
        return -EINVAL;
    
    mtx->m_locked   = 0;
    mtx->m_owner    = NULL;
    mtx->m_recurs   = 0;
    spinlock_init(&mtx->m_guard);
    return queue_init(&mtx->m_waitQ);
}

void mtx_lock(mtx_t *mtx) {
    mtx_assert(mtx);

    spin_lock(&mtx->m_guard);

    assert(!(mtx->m_locked && mtx->m_owner == current), "Already held mtx.\n");
    if (mtx->m_locked) {
        sched_wait(&mtx->m_waitQ, T_SLEEP, QUEUE_TAIL, &mtx->m_guard);
    }

    mtx->m_locked = 1;
    mtx->m_recurs = 1;
    mtx->m_owner  = current;

    spin_unlock(&mtx->m_guard);
}

void mtx_unlock(mtx_t *mtx) {
    mtx_assert(mtx);

    spin_lock(&mtx->m_guard);

    assert(mtx->m_locked && mtx->m_owner == current, "Not holding mtx.\n");
    assert(mtx->m_recurs > 0, "Mtx invalid recursion.\n");

    if (--mtx->m_recurs == 0) {
        mtx->m_locked   = 0;
        mtx->m_owner    = NULL;
        sched_wakeup(&mtx->m_waitQ, WAKEUP_NORMAL, QUEUE_HEAD);
    }

    spin_unlock(&mtx->m_guard);
}

int mtx_trylock(mtx_t *mtx) {
    int success = 0;

    mtx_assert(mtx);
    spin_lock(&mtx->m_guard);

    assert(!(mtx->m_locked && mtx->m_owner == current), "Already held mtx.\n");

    if (mtx->m_locked == 0) {
        success         = 1;
        mtx->m_locked   = 1;
        mtx->m_recurs   = 1;
        mtx->m_owner    = current;
    }
    spin_unlock(&mtx->m_guard);

    return success;
}

int mtx_islocked(mtx_t *mtx) {
    mtx_assert(mtx);

    spin_lock(&mtx->m_guard);
    int locked = mtx->m_locked && mtx->m_owner == current;
    spin_unlock(&mtx->m_guard);
    return locked;
}

void mtx_assert_locked(mtx_t *mtx) {
    assert(mtx_islocked(mtx), "Must hold mtx.\n");
}

void mtx_recursive_lock(mtx_t *mtx) {
    mtx_assert(mtx);

    spin_lock(&mtx->m_guard);

    if (mtx->m_locked && mtx->m_owner != current) {
        sched_wait(&mtx->m_waitQ, T_SLEEP, QUEUE_TAIL, &mtx->m_guard);
    }

    mtx->m_locked = 1;
    mtx->m_recurs+= 1;
    mtx->m_owner  = current;

    spin_unlock(&mtx->m_guard);
}
