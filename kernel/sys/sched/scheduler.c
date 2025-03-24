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

static MLFQ_t MLFQ[NCPU];

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

static MLFQ_t *MLFQ_get(void) {
    return &MLFQ[getcpuid()];
}

static MLFQ_level_t *MLFQ_get_level(MLFQ_t *mlfq, int i) {
    if (!mlfq || i < MLFQ_LOW || i > MLFQ_HIGH) {
        return NULL;
    }

    return &mlfq->level[i];
}

static void MLFQ_init(void) {
    usize   quantum = 0;
    MLFQ_t  *mlfq   = MLFQ_get();

    memset(mlfq, 0, sizeof *mlfq);

    quantum = jiffies_from_ms(30);
    foreach_level(mlfq) {
        level->quantum = quantum;
        quantum -= jiffies_from_ms(5);
    }
}

void scheduler_init(void) {
    MLFQ_init();
}

static usize MLFQ_load(MLFQ_t *mlfq) {
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
    MLFQ_t       *mlfq  = NULL;
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
     * @brief SEE: Lock ordering used in MLFQ_get_next().
     * @brief THOUGHT: But then again, thread isn't in queue yet,
     * so I suppose it is still safe to acquire locks in this order:
     * thread->t_lock -> thread_queue->lock -> thread_queue->queue->lock.*/
    thread_assert_locked(thread);

    mlfq = MLFQ_get();

    if (thread->t_info.ti_sched.ts_affin.type == HARD_AFFINITY) {
        usize mlfq_load = MLFQ_load(MLFQ_get()); // get current cpu's load.
        // Choose suitable MLFQ.
        for (int i = 0; i < ncpu(); ++i) {
            if (MLFQ_get() == &MLFQ[i]) {
                continue; // skip self MLFQ.
            }

            // Choose a MLFQ which is the least loaded among the CPU affinity set.
            if (thread->t_info.ti_sched.ts_affin.cpu_set & (1 << i)) {
                if (mlfq_load >= MLFQ_load(&MLFQ[i])) {
                    mlfq = &MLFQ[i]; // pick MLFQ with lower load.
                }
            }
        }
    }

    // Enqueue thread according to it's current priority level.
    level = MLFQ_get_level(mlfq, thread_get_prio(thread));
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

static thread_t *MLFQ_get_next(void) {
    int     err   = 0;
    MLFQ_t  *mlfq = MLFQ_get();

    foreach_level(mlfq) {
        /// Acquire level resources.
        queue_lock(&level->run_queue);
        embedded_queue_foreach(&level->run_queue, thread_t, thread, t_run_qnode) {
            thread_lock(thread);

            /// Remove thread from run queue.
            /// panic is this fails. What could possibly go run?
            assert_eq(err = embedded_queue_detach(&level->run_queue, thread_node), 0,
                "Failed to remove thread[%d:%d] at priority level: %s: Error: %s\n",
                thread_getpid(thread), thread_gettid(thread), MLFQ_PRIORITY[level - mlfq->level], perror(err)
            );

            thread->t_info.ti_sched.ts_timeslice = level->quantum;

            /// Ensure we release level resources.
            queue_unlock(&level->run_queue);

            /* Update scheduling metadata for the chosen thread. */
            thread->t_info.ti_sched.ts_proc      = cpu; // set the current processor for chosen thread.

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

int sched_enqueue(thread_t *thread) {
    if (thread == NULL) {
        return -EINVAL;
    }

    /// All thread start at the highest priority level.
    thread_set_prio(thread, MLFQ_HIGH);
    thread_enter_state(thread, T_READY);
    return MLFQ_enqueue(thread);
}

__unused static void MLFQ_pull(void) {
    usize           count           = 0;
    usize           target_load     = 0;
    usize           count_pulled    = 0;
    MLFQ_t          *target_mlfq    = NULL;
    MLFQ_t          *current_mlfq   = NULL;

    current_mlfq = MLFQ_get(); // get current MLFQ.

    /// Find the most loaded MLFQ
    foreach_MLFQ() {
        usize load = 0;
        if (mlfq == current_mlfq) {
            continue;
        }

        if (target_load < (load = MLFQ_load(mlfq))) {
            target_load = load;
            target_mlfq = mlfq;
        }
    }

    if (target_mlfq == NULL || target_load < 2) {
        return;
    }

    /// Steal thread in reversed order of priority.
    /// This is because the owning CPU, will always check the highest
    /// priority level first then lower levels.
    /// This inturn may reduce contention for run queues.
    foreach_level_reverse(target_mlfq) {
        MLFQ_level_t    *target_level   = NULL;
        /// We have stolen enough threads, break out of loop.
        if (count_pulled >= (target_load / 2)) {
            break;
        }

        /// No coresponding thread.
        target_level = MLFQ_get_level(current_mlfq, level - target_mlfq->level);
        assert(target_level, "Invalid target level.\n");

        // Attempt to acquire locks.
        loop() {
            if (queue_trylock(&target_level->run_queue)) {
                if (queue_trylock(&level->run_queue)) {
                    break;
                }
                queue_unlock(&target_level->run_queue);  // Release first lock before continuing.
            }
            jiffies_timed_wait(2);
        }

        // Successfully acquired both locks, proceed with migration.
        switch (queue_length(&level->run_queue)) {
        case 0: // Skip empty run queues.
            queue_unlock(&level->run_queue);
            queue_unlock(&target_level->run_queue);
            continue;
        case 1: /// For run queues having only 1 thread take it,
            /// if we haven't reached total number needed.
            if (count_pulled < (target_load / 2))
                count =  1;
            break;
        default:
            count = queue_length(&level->run_queue) / 2;
        }

        int err = embedded_queue_migrate(
            &target_level->run_queue,
            &level->run_queue,
            queue_length(&level->run_queue) / 2,
            count,
             QUEUE_TAIL
        );

        assert_eq(err, 0, "Error[%s]: Failed to migrate threads\n", perror(err));
        count_pulled += count;
 
        // Unlock in the reverse order of locking
        queue_unlock(&level->run_queue);
        queue_unlock(&target_level->run_queue);
        // debug("load: %d, stealing: %d, stolen: %d to prio: %s\n",
        //    load, count,  count_pulled, MLFQ_PRIORITY[target_level - MLFQ_get()->level]);
    }
}

static void MLFQ_push(void) {
    usize           target_load     = 0;
    usize           count_pushed    = 0;
    usize           count           = 0;
    usize           my_load         = 0;
    MLFQ_t          *target_mlfq    = NULL;
    MLFQ_t          *current_mlfq   = NULL;

    current_mlfq = MLFQ_get(); // get current MLFQ.

    foreach_MLFQ() {
        if (mlfq == current_mlfq) {
            continue;
        }

        usize load = MLFQ_load(mlfq);
        if (target_load >= load) {
            target_load = load;
            target_mlfq = mlfq;
        }
    }

    /** Return if no suitable core is found or if target
     * core has a load greater or equal to current core's load*/
    if (target_mlfq == NULL || target_load >= MLFQ_load(current_mlfq)) {
        return;
    }

    foreach_level(current_mlfq) {
        MLFQ_level_t    *target_level   = NULL;

        target_level = MLFQ_get_level(target_mlfq, level - current_mlfq->level);
        assert(target_level, "Invalid target level.\n");

        if (queue_trylock(&level->run_queue)) {
            if (queue_trylock(&target_level->run_queue) == 0) {
                // Locks not avalable.
                queue_lock(&level->run_queue);
                /**
                 * @brief Spinning for the locks here is not desirable.
                 * 
                 * Unlike in MLFQ_pull(), a processor calling to push threads
                 * is already bomberdded with work, so waiting would significantly reduce performance.
                 * 
                 * So just continue, or maybe return. Which is better?
                 * */
                continue;
            }
        }

        if ((count = queue_length(&level->run_queue)) >= 2) {
            count /= 2;
        } else if ((count == 0) || (count_pushed >= (my_load / 2))) {
            queue_unlock(&level->run_queue);
            queue_unlock(&target_level->run_queue);
            continue;
        }

        int err = embedded_queue_migrate(
            &target_level->run_queue,
            &level->run_queue,
            queue_length(&level->run_queue) / 2,
            count,
            QUEUE_HEAD
        );
        assert_eq(err, 0, "Error[%s]: Failed to migrate threads\n", perror(err));

        count_pushed += count;

        queue_unlock(&level->run_queue);
        queue_unlock(&target_level->run_queue);
        // debug("load: %d, pushing: %d, pushed: %d to prio: %s\n",
        //     my_load, count, count_pushed, MLFQ_PRIORITY[target_level - target_mlfq->level]);
    }
}

static void MLFQ_balance(void) {
    MLFQ_push();
}

static void sched_handle_zombie(void) {
    cond_broadcast(&current->t_event);
}

static void hanlde_thread_state(void) {
    int err = 0;

    switch (current_get_state()) {
        case T_EMBRYO:
            assert(0, "T_EMBRYO thread returned?\n");
            break;
        case T_RUNNING:
            assert(0, "T_RUNNING thread returned?\n");
            break;
        case T_READY:
            assert_eq(err = MLFQ_enqueue(current), 0,
                "Failed to enqueue current thread. error: %s\n", perror(err));
            break;
        case T_SLEEP:
        case T_STOPPED:
            break;
        case T_ZOMBIE:
            sched_handle_zombie();
            break;
        case T_TERMINATED:
            todo("Please Handle: %s\n", get_tstate());
            break;
        default:
        todo("Undefined state: %s\n", get_tstate());
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
            if (set_current(MLFQ_get_next())) {
                break;
            }

            hlt();
        }

        sched_update_thread_metrics(current);

        // jmp to thread.
        context_switch(&current->t_arch.t_context);

        current_assert_locked();
        sched_update_thread_metrics(current);
        hanlde_thread_state();
    }
}

__noreturn void scheduler_load_balancer(void) {
    sigset_t set;
    sigsetfill(&set);
    sigsetdelmask(&set, SIGMASK(SIGKILL) | SIGMASK(SIGSTOP));
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    loop() {
        if ((jiffies_get() % (SYS_Hz)) == 0) {
            MLFQ_balance();
        }

        cpu_pause();
        sched_yield();
    }

} BUILTIN_THREAD(scheduler_load_balancer, scheduler_load_balancer, NULL);