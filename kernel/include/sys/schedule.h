#pragma once

// Core includes for timekeeping, synchronization, and data structures
#include <dev/timer.h>
#include <ds/queue.h>
#include <sync/spinlock.h>

// Definition of a scheduling level in the MLFQ
typedef struct {
    jiffies_t   quantum;    // Time quantum for this level
    queue_t     run_queue;  // Queue to hold processes for this level.
} MLFQ_level_t;

typedef struct {
    uint64_t total_context_switches;
    uint64_t total_threads_executed;
    uint64_t last_idle_time;            // Time CPU spent idle.
    uint64_t last_active_time;
    uint64_t load;
    uint64_t idle;
} sched_metrics_t;

extern sched_metrics_t *get_metrics(void);
extern sched_metrics_t *get_cpu_metrics(int core);

// Number of scheduling levels in the MLFQ
#define NSCHED_LEVEL 4

// Definition of the Multi-Level Feedback Queue (MLFQ) structure
typedef struct {
    sched_metrics_t metrics;
    MLFQ_level_t    level[NSCHED_LEVEL]; // Array of scheduling levels
} MLFQ_t;

// Highest priority level.
#define MLFQ_HIGH    (NSCHED_LEVEL - 1)

#define AGING_THRESHOLD      5    // Boost after 5 scheduler cycles
#define AGING_INTERVAL_MS    100  // Run aging every 100ms
#define SOFT_AFFINITY_BIAS   2    // Prefer current CPU unless others have 2+ less load

// Lowest priority level.
#define MLFQ_LOW     0

extern const char *MLFQ_PRIORITY[];

extern void sched(void);
extern void scheduler_init(void);
extern void scheduler_tick(void);
extern int sched_enqueue(thread_t *thread);

extern __noreturn void scheduler(void);

/**
 * @brief give up the cpu for one scheduling round
 *
 */
extern void sched_yield(void);

extern void toggle_sched_monitor(void);

extern int sched_wait(queue_t *wait_queue, tstate_t state, spinlock_t *lock);
extern int sched_wait_whence(queue_t *wait_queue, tstate_t state, queue_relloc_t whence, spinlock_t *lock);

typedef enum {
    WAKEUP_NONE     = 0,
    WAKEUP_NORMAL   = 1, // Normal wakeup.
    WAKEUP_SIGNAL   = 2, // Wakeup due to signal.
    WAKEUP_TIMEOUT  = 3, // Wakeup due to timeout.
    WAKEUP_ERROR    = 4, // Wakeup due to error.
} wakeup_t;

static inline int wakeup_reason_validate(wakeup_t reason) {
    return (reason != WAKEUP_NORMAL &&
            reason != WAKEUP_SIGNAL &&
            reason != WAKEUP_TIMEOUT) ? 0 : 1;
}

extern usize sched_wait_queue_length(queue_t *wait_queue);
extern int sched_wakeup_all(queue_t *wait_queue, wakeup_t reason, size_t *pnt);
extern int sched_wakeup_specific(queue_t *wait_queue, wakeup_t reason, tid_t tid);
extern int sched_wakeup(queue_t *wait_queue, wakeup_t reason, queue_relloc_t whence);
extern int sched_detach_and_wakeup(queue_t *wait_queue, thread_t *thread, wakeup_t reason);

/**
 * wait_event_interruptible - sleep until a condition gets true
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @lock: optional spinlock to release while sleeping
 *
 * The process is put to sleep (T_SLEEP) until the
 * @condition evaluates to true. The @condition is checked each time
 * the waitqueue @wq is woken up.
 *
 * Return: 0 if the @condition evaluated to true after sleep, -ERESTART
 * if it was interrupted by a signal.
 */
#define wait_event_interruptible(wq, condition, lock) ({    \
    int __ret = 0;                                          \
    if (!(condition)) {                                     \
        __ret = sched_wait_whence(wq, T_SLEEP, QUEUE_TAIL, lock);  \
    }                                                       \
    __ret;                                                  \
})
