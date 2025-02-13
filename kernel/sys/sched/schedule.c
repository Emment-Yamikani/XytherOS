#include <core/debug.h>
#include <string.h>
#include <limits.h>
#include <sys/schedule.h>
#include <sys/thread.h>

#define foreach_MLFQ() \
    for (MLFQ_t *mlfq = &MLFQ[0]; mlfq < &MLFQ[NELEM(MLFQ)]; ++mlfq)

// iterates through the MLFQ from highest to lowest priority level.
#define foreach_level(mlfq)                                                  \
    for (MLFQ_level_t *level = (mlfq) ? &(mlfq)->level[MLFQ_HIGHEST] : NULL; \
         level >= &(mlfq)->level[MLFQ_LOWEST]; --level)

// same as foreach_level but in reverse
#define foreach_level_reverse(mlfq)                                         \
    for (MLFQ_level_t *level = (mlfq) ? &(mlfq)->level[MLFQ_LOWEST] : NULL; \
         level <= &(mlfq)->level[MLFQ_HIGHEST]; ++level)

const char *MLFQ_PRIORITY[] = {
    [MLFQ_LOWEST]       = "LOWEST",
    [MLFQ_LOWEST + 1]   = "MID-LEVEL1",
    [MLFQ_LOWEST + 2]   = "MID-LEVEL2",
    [MLFQ_HIGHEST]      = "HIGHEST",
    [NSCHED_LEVEL]      = NULL
};

static MLFQ_t MLFQ[NCPU];

static MLFQ_t *MLFQ_get(void) {
    return &MLFQ[getcpuid()];
}

static MLFQ_level_t *MLFQ_get_level(MLFQ_t *mlfq, int i) {
    if (!mlfq || i < MLFQ_LOWEST || i > MLFQ_HIGHEST)
        return NULL;
    return &mlfq->level[i];
}

static void MLFQ_init(void) {
    MLFQ_t *mlfq = MLFQ_get();

    memset(mlfq, 0, sizeof *mlfq);

    usize quantum = jiffies_from_ms(10);
    for (int i = MLFQ_HIGHEST; i >= MLFQ_LOWEST; --i) {
        mlfq->level[i].quantum = quantum;
        quantum += jiffies_from_ms(10);
    }
}

void scheduler_init(void) {
    MLFQ_init();
}

static usize MLFQ_load(MLFQ_t *mlfq) {
    usize load = 0;

    if (mlfq == NULL)
        return 0;
    
    foreach_level(mlfq) {
        queue_lock(&level->run_queue);
        load += queue_count(&level->run_queue);
        queue_unlock(&level->run_queue);
    }

    return load;
}

static int MLFQ_enqueue(thread_t *thread) {
    int          err = 0;
    MLFQ_t      *mlfq = NULL;
    MLFQ_level_t *level = NULL;

    if (thread == NULL)
        return -EINVAL;

    /**
     * @brief FIXME: This violates the lock ordering described below.
     *
     * thread_queue->lock before thread_queue->queue->lock before thread->t_lock.
     *
     * Is it safe to momentarily release thread->t_lock,
     * then re-acquire it after acquisition of:
     * thread_queue->lock before thread_queue->queue->lock.
     *
     * @brief SEE: lock ordering used in MLFQ_get_next().
     *
     * @brief THOUGHT: but then again, thread isn't in queue yet,
     * so I suppose it is still safe to acquire locks in this order:
     * thread->t_lock -> thread_queue->lock -> thread_queue->queue->lock.*/
    thread_assert_locked(thread);

    mlfq = MLFQ_get();

    if (thread->t_info.ti_sched.ts_affin.type == HARD_AFFINITY) {
        usize mlfq_load = MLFQ_load(MLFQ_get()); // get current cpu's load.

        // Choose suitable MLFQ.
        for (int i = 0; i < ncpu(); ++i) {
            if (MLFQ_get() == &MLFQ[i]) // skip self MLFQ.
                continue;

            // Choose a MLFQ which is the least loaded among the CPU affinity set.
            if (thread->t_info.ti_sched.ts_affin.cpu_set & (1 << i)) {
                if (mlfq_load >= MLFQ_load(&MLFQ[i])) {
                    mlfq = &MLFQ[i]; // pick MLFQ with lower load.
                }
            }
        }
    }

    // enqueue thread according to it's current priority level.
    level = MLFQ_get_level(mlfq, thread_get_prio(thread));
    if (level == NULL) {
        return -EINVAL;
    }

    queue_lock(&level->run_queue);

    if ((err = embedded_enqueue(&level->run_queue, &thread->t_run_qnode, QUEUE_ENFORCE_UNIQUE))) {
        // ensure we release level resources.
        queue_unlock(&level->run_queue);
        return err;
    }

    // ensure we release level resources.
    queue_unlock(&level->run_queue);

    thread_enter_state(thread, T_READY);

    return 0;
}

static thread_t *MLFQ_get_next(void) {
    int     err   = 0;
    MLFQ_t  *mlfq = MLFQ_get();
    
    foreach_level(mlfq) {
        // acquire level resources.
        queue_lock(&level->run_queue);

        embedded_queue_foreach(&level->run_queue, thread_t, thread, t_run_qnode) {
            thread_lock(thread);
            /// remove thread from run queue.
            /// panic is this fails.
            /// What could possibly go run?
            assert_eq(err = embedded_queue_detach(&level->run_queue, thread_node), 0,
                "Failed to remove thread[%d:%d] at priority level: %s: Error: %s\n",
                thread_getpid(thread), thread_gettid(thread), MLFQ_PRIORITY[level - mlfq->level], perror(err)
            );

            // ensure we release level resources.
            queue_unlock(&level->run_queue);

            /* update scheduling metadata for the chosen thread. */

            thread->t_info.ti_sched.ts_proc       = cpu; // set the current processor for chosen thread.
            thread->t_info.ti_sched.ts_last_sched = jiffies_get();
            thread->t_info.ti_sched.ts_timeslice  = level->quantum;

            // enter running state.
            thread_enter_state(thread, T_RUNNING);
            // success, return thread for execution.
            return thread;
        }

        // no thread was found at this level.
        queue_unlock(&level->run_queue);
    }

    // no threads ready to run on this cpu-core.
    return NULL;
}

int sched_enqueue(thread_t *thread) {
    if (thread == NULL)
        return -EINVAL;
    // All thread start at the highest priority level.
    thread_set_prio(thread, MLFQ_HIGHEST);
    return MLFQ_enqueue(thread);
}

static void MLFQ_steal_threads(void) {
    usize           max_load          = 0;
    usize           stolen_count      = 0;
    usize           count_to_steal    = 0;
    MLFQ_level_t    *dst_level        = NULL;
    MLFQ_t          *most_loaded_MLFQ = NULL;

    // Find the most loaded MLFQ
    foreach_MLFQ() {
        usize load = 0;

        if (mlfq == MLFQ_get())
            continue;

        if (max_load < (load = MLFQ_load(mlfq))) {
            max_load    = load;
            most_loaded_MLFQ= mlfq;
        }
    }


    if (most_loaded_MLFQ == NULL || max_load < 2)
        return;

    /// steal thread in reversed order of priority.
    /// this is because the owning CPU, will always check the highest
    /// priority level first then lower levels.
    /// This inturn may reduce contention for run queues.
    foreach_level_reverse(most_loaded_MLFQ) {
        // We have stolen enough threads, break out of loop.
        if (stolen_count >= (max_load / 2))
            break;

        // No coresponding thread.
        if (NULL ==  (dst_level = MLFQ_get_level(MLFQ_get(), level - most_loaded_MLFQ->level))) {
            continue;
        }

        // Attempt to acquire locks.
        loop() {
            if (queue_trylock(&dst_level->run_queue) == 0) {
                if (queue_trylock(&level->run_queue) == 0) {
                    break;
                }
                queue_unlock(&dst_level->run_queue);  // Release first lock before continuing.
            }
            
            jiffies_timed_wait(s_from_us(20));
        }

        // Successfully acquired both locks, proceed with migration.
        switch (queue_count(&level->run_queue)) {
        case 0: // Skip empty run queues.
            queue_unlock(&level->run_queue);
            queue_unlock(&dst_level->run_queue);
            continue;
        case 1: /// For run queues having only 1 thread take it,
            /// if we haven't reached total number needed.
            if (stolen_count < (max_load / 2))
                count_to_steal = 1;
            break;
        default:
            count_to_steal = queue_count(&level->run_queue) / 2;
        }

        int err = embedded_queue_migrate(
            &dst_level->run_queue,
            &level->run_queue,
            queue_count(&level->run_queue) / 2,
            count_to_steal,
            QUEUE_RELLOC_TAIL
        );

        assert_eq(err, 0, "Error[%s]: Failed to migrate threads\n", perror(err));

        stolen_count += count_to_steal;

        // Unlock in the reverse order of locking
        queue_unlock(&level->run_queue);
        queue_unlock(&dst_level->run_queue);
        debug("max_load: %d, stealing: %d, stolen: %d to prio: %s\n",
            max_load, count_to_steal, stolen_count, MLFQ_PRIORITY[dst_level - MLFQ_get()->level]);
    }
}

void sched(void) {
    current_assert_locked();

    if (current_test(THREAD_WAKE)) {
        current_mask(THREAD_WAKE | THREAD_PARK);
        return;
    }

    disable_preemption();

    isize ncli   = cpu->ncli;
    isize intena = cpu->intena;

    /// used up entire timesclice, so downgrade it one priority level.
    if (current->t_info.ti_sched.ts_timeslice == 0) {
        // Check to prevent underflow.
        if (current->t_info.ti_sched.ts_prio != 0)
            current->t_info.ti_sched.ts_prio -= 1;
    }

    // return to the sscheduler.
    context_switch(&current->t_arch.t_ctx);

    cpu->intena = intena;
    cpu->ncli   = ncli;

    enable_preemption();
}

void scheduler_tick(void) {

}

static void hanlde_thread_state(void) {
    int err = 0;

    switch (current_get_state()) {
        case T_EMBRYO:
            assert(0, "T_EMBRYO thread returned?\n");
            break;
        case T_READY:
            assert(0, "T_READY thread returned?\n");
            break;
        case T_RUNNING:
            assert_eq(err = MLFQ_enqueue(current), 0,
                "Failed to enqueue current thread. error: %s\n", perror(err)
            );
            break;
        case T_SLEEP:
        case T_STOPPED:
            break;
        case T_ZOMBIE:
        case T_TERMINATED:
            // does nothing special yet.
            todo("Please Handle: %s\n", get_tstate());
            break;
        default:
        assert(0, "Undefined state: %s\n", get_tstate());
    }

    current_unlock();
}

__noreturn void scheduler(void) {
    loop() {
        cpu->ncli   = 0;
        cpu->intena = 0;
        set_current(NULL);

        sti();

        loop() {

            set_current(MLFQ_get_next());
            if (current)
                break;

            MLFQ_steal_threads();

            set_current(MLFQ_get_next());

            if (current)
                break;

            hlt();
        }

        // jmp to thread.
        context_switch(&current->t_arch.t_ctx);

        disable_preemption();

        hanlde_thread_state();
    }
}
