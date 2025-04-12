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
    uint64_t total_cpu_time;       // Total CPU time across all threads.
    uint64_t total_wait_time;      // Total wait time for all threads.
    uint64_t total_threads_executed;
    uint64_t idle_time;            // Time CPU spent idle.
    uint64_t preemption_count;
    uint64_t steal_attempts;
    uint64_t successful_steals;
} sched_metrics_t;

// Number of scheduling levels in the MLFQ
#define NSCHED_LEVEL 4

// Definition of the Multi-Level Feedback Queue (MLFQ) structure
typedef struct {
    sched_metrics_t metrics;
    MLFQ_level_t    level[NSCHED_LEVEL]; // Array of scheduling levels
} MLFQ_t;

// Highest priority level.
#define MLFQ_HIGH    (NSCHED_LEVEL - 1)

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

extern int sched_wait(queue_t *wait_queue, tstate_t state, queue_relloc_t whence, spinlock_t *lock);

typedef enum {
    WAKEUP_NONE     = 0,
    WAKEUP_NORMAL   = 1, // Normal wakeup.
    WAKEUP_SIGNAL   = 2, // Wakeup due to signal.
    WAKEUP_TIMEOUT  = 3, // Wakeup due to timeout.
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