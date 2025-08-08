#pragma once

#include <sys/schedule.h>
#include <sys/thread.h>

extern MLFQ_t MLFQ[NCPU];

#define foreach_MLFQ() \
    for (MLFQ_t *mlfq = &MLFQ[0]; mlfq < &MLFQ[ncpu()]; ++mlfq)

// iterates through the MLFQ from highest to lowest priority level.
#define foreach_level(mlfq)                                               \
    for (MLFQ_level_t *level = (mlfq) ? &(mlfq)->level[MLFQ_HIGH] : NULL; \
         level >= &(mlfq)->level[MLFQ_LOW]; --level)

// same as foreach_level but in reverse
#define foreach_level_reverse(mlfq)                                      \
    for (MLFQ_level_t *level = (mlfq) ? &(mlfq)->level[MLFQ_LOW] : NULL; \
         level <= &(mlfq)->level[MLFQ_HIGH]; ++level)

static inline MLFQ_t *MLFQ_get(void) {
    return &MLFQ[getcpuid()];
}

static inline MLFQ_level_t *MLFQ_get_level(MLFQ_t *mlfq, int i) {
    if (!mlfq || i < MLFQ_LOW || i > MLFQ_HIGH) {
        return NULL;
    }

    return &mlfq->level[i];
}

extern void MLFQ_push(void);
extern void MLFQ_pull(void);
extern usize MLFQ_load(MLFQ_t *mlfq);
extern void MLFQ_balance(void);
extern MLFQ_t *MLFQ_most_loaded(void);
extern MLFQ_t *MLFQ_least_loaded(void);

extern void sched_update_thread_metrics(thread_t *thread);
