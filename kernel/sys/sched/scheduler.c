#include "metrics.h"
#include <core/debug.h>
#include <limits.h>
#include <string.h>
#include <sys/schedule.h>
#include <sys/thread.h>

const char *MLFQ_PRIORITY[] = {
    [MLFQ_LOW]    = "LOWEST",
    [MLFQ_LOW+1]  = "MID-L1",
    [MLFQ_LOW+2]  = "MID-L2",
    [MLFQ_HIGH]   = "HIGHEST",
    [NSCHED_LEVEL]= NULL
};

MLFQ_t MLFQ[NCPU];

static void MLFQ_init(void) {
    usize   t = 0;
    MLFQ_t  *mlfq   = MLFQ_get();

    for (int i = 0; i < ncpu(); ++i) {
        sched_metrics_t *metrics = get_cpu_metrics(i);
        memset(metrics, 0, sizeof *metrics);
    }

    memset(mlfq, 0, sizeof *mlfq);

    t = jiffies_from_ms(10);
    int l = 0; // implicit level: 0 is highest

    foreach_level(mlfq) {
        // Q = t + (5 * 2^n), Q is the allocated quantum,
        // t is the base quantum where n is the level index.
        level->quantum = t  + (5 * (1 < l++));
    }
}

void scheduler_init(void) {
    MLFQ_init();
}

usize MLFQ_load(MLFQ_t *mlfq) {
    usize load = 0;

    if (mlfq == NULL) {
        return 0;
    }

    foreach_level(mlfq) {
        queue_lock(&level->run_queue);
        load += queue_length(&level->run_queue);
        queue_unlock(&level->run_queue);
    }

    return load;
}

static int MLFQ_enqueue(thread_t *thread) {
    int          err    = 0;
    MLFQ_level_t *level = NULL;
    
    if (thread == NULL) {
        return -EINVAL;
    }
    
    /**
     * @brief FIXME: This violates the lock ordering described below.
     * thread_queue->lock before thread_queue->queue->lock before thread->t_lock.
     *
     * Is it safe to momentarily release thread->t_lock,
     * then re-acquire it after acquisition of:
     * thread_queue->lock before thread_queue->queue->lock.
     *
     * @brief SEE: Lock ordering used in MLFQ_get_next_thread().
     * @brief THOUGHT: But then again, thread isn't in queue yet,
     * so I suppose it is still safe to acquire locks in this order:
     * thread->t_lock -> thread_queue->lock -> thread_queue->queue->lock.*/
    thread_assert_locked(thread);

    MLFQ_t *target = MLFQ_get();
    cpu_affin_t *affin = &thread->t_info.ti_sched.ts_affin;

    if (affin->type == HARD_AFFINITY) {
        usize least = MLFQ_load(target); // get current cpu's load.
        // Choose suitable MLFQ among CPUs for which we have affinity.
        foreach_MLFQ() {
            const int core = mlfq - MLFQ;
            if (affin->cpu_set & (1 << core)) {
                MLFQ_t *other = &MLFQ[core];
                const usize load = MLFQ_load(other);
                if (load <= least) {
                    least   = load;
                    target  = other;
                }
            }
        }
    } else { // Soft affinity.
        target = MLFQ_least_loaded();
    }

    // Enqueue thread according to it's current priority level.
    level = MLFQ_get_level(target, thread_get_prio(thread));
    if (level == NULL) {
        return -EINVAL;
    }

    queue_lock(&level->run_queue);
    if ((err = embedded_enqueue(&level->run_queue, &thread->t_run_qnode, QUEUE_UNIQUE))) {
        // Ensure we release level resources.
        queue_unlock(&level->run_queue);
        return err;
    }

    // Ensure we release level resources.
    queue_unlock(&level->run_queue);
    return 0;
}

int sched_enqueue(thread_t *thread) {
    if (thread == NULL) {
        return -EINVAL;
    }

    /// All thread start at the highest priority level.
    thread_set_prio(thread, MLFQ_HIGH);
    thread_enter_state(thread, T_READY);
    return MLFQ_enqueue(thread);
}

static thread_t *MLFQ_get_next_thread(void) {
    int     err   = 0;
    thread_t *thread;
    MLFQ_t  *mlfq = MLFQ_get();

    foreach_level(mlfq) {
        /// Acquire level resources.
        queue_lock(&level->run_queue);
        foreach_thread(&level->run_queue, thread, t_run_qnode) {
            thread_lock(thread);

            /// Remove thread from run queue.
            /// panic is this fails. What could possibly go run?
            assert_eq(err = embedded_queue_detach(&level->run_queue, thread_node), 0,
                "Failed to remove thread[%d:%d] at priority level: %s: Error: %s\n",
                thread_getpid(thread), thread_gettid(thread), MLFQ_PRIORITY[level - mlfq->level], strerror(err)
            );

            thread->t_info.ti_sched.ts_timeslice = level->quantum;

            /// Ensure we release level resources.
            queue_unlock(&level->run_queue);

            /* Update scheduling metadata for the chosen thread. */
            thread->t_info.ti_sched.ts_proc = cpu; // set the current processor for chosen thread.

            /// Enter running state.
            thread_enter_state(thread, T_RUNNING);

            /// Success, return thread for execution.
            return thread;
        }

        /// No thread was found at this level.
        queue_unlock(&level->run_queue);
    }

    /// No threads ready to run on this cpu-core.
    return NULL;
}

static void sched_handle_zombie(thread_t *thread) {
    cond_broadcast(&thread->t_event);
}

static void hanlde_thread_state(thread_t *thread) {
    int err = 0;

    switch (thread_get_state(thread)) {
        case T_EMBRYO:
            assert(0, "T_EMBRYO thread returned?\n");
            break;
        case T_RUNNING:
            assert(0, "T_RUNNING thread returned?\n");
            break;
        case T_READY:
            assert_eq(err = MLFQ_enqueue(thread), 0,
                "Failed to enqueue current thread. error: %s\n", strerror(err));
            break;
        case T_SLEEP: case T_STOPPED: break;
        case T_ZOMBIE:
            sched_handle_zombie(thread);
            break;
        case T_TERMINATED:
            todo("Please Handle: %s\n", get_tstate());
            break;
        default:
            todo("Undefined state: %s\n", get_tstate());
    }

    thread_unlock(thread);
}

// this is the per-cpu scheduler's idle thread, well, somewhat.
__noreturn void scheduler(void) {
    thread_t *thread;
    MLFQ_t   *my_mlfq = MLFQ_get();
    sched_metrics_t *metrics = get_metrics();

    loop() {
        cpu_set_preepmpt(0, 0);
        cpu_set_thread(NULL);

        atomic_set(&metrics->load, MLFQ_load(my_mlfq));

        enable_interrupts(true);

        loop() {
            thread = MLFQ_get_next_thread();
            if (thread && cpu_set_thread(thread)) {
                /// set cpu->intena == false,
                /// might need a better way of preventing undefined behavior
                /// caused by current_unlock() in arch_thread_start()
                /// when exec-ing a thread for the first time.
                cpu_set_intena(false);
                atomic_clear(&metrics->idle);
                atomic_inc(&metrics->total_threads_executed);
                break;
            }

            atomic_set(&metrics->idle, 1);
            atomic_set(&metrics->last_idle_time, jiffies_get());

            hlt();
        }

        sched_update_thread_metrics(current);

        atomic_set(&metrics->last_active_time, jiffies_get());

        // jmp to thread.
        context_switch(&current->t_arch.t_context);

        atomic_inc(&metrics->total_context_switches);

        thread_assert_locked(thread);
        sched_update_thread_metrics(thread);
        hanlde_thread_state(thread);

        assert(!intrena(), "Interrupts are enabled???\n");
    }
}