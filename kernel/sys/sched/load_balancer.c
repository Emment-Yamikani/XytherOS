#include "metrics.h"
#include <core/debug.h>

void MLFQ_adjust_timeslice(MLFQ_t *) {
}

MLFQ_t *MLFQ_most_loaded(void) {
    MLFQ_t *most_loaded = MLFQ_get();
    usize highest = MLFQ_load(most_loaded);

    foreach_MLFQ() {
        const usize load = MLFQ_load(mlfq);
        if (load >= highest) {
            highest = load;
            most_loaded = mlfq;
        }
    }

    return most_loaded;
}

MLFQ_t *MLFQ_least_loaded(void) {
    MLFQ_t *least_loaded = MLFQ_get();
    usize least = MLFQ_load(least_loaded);

    foreach_MLFQ() {
        const usize load = MLFQ_load(mlfq);
        if (load <= least) {
            least = load;
            least_loaded = mlfq;
        }
    }

    return least_loaded;
}

void MLFQ_pull(void) {
    usize  count           = 0;
    usize  target_load     = 0;
    usize  count_pulled    = 0;
    MLFQ_t *target_mlfq    = NULL;
    MLFQ_t *current_mlfq   = NULL;

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
        if (!queue_trylock(&target_level->run_queue)) {
            continue;
        }
        
        if (!queue_trylock(&level->run_queue)) {
            queue_unlock(&target_level->run_queue);  // Release first lock before continuing.
            continue;
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
            count, QUEUE_TAIL
        );

        if (err != 0) {
            debug("Error[%s]: Failed to migrate threads\n", strerror(err));
            queue_unlock(&level->run_queue);
            queue_unlock(&target_level->run_queue);
            continue;
        }

        count_pulled += count;
 
        // Unlock in the reverse order of locking
        queue_unlock(&level->run_queue);
        queue_unlock(&target_level->run_queue);
        // debug("load: %d, stealing: %d, stolen: %d to prio: %s\n",
        //    load, count,  count_pulled, MLFQ_PRIORITY[target_level - MLFQ_get()->level]);
    }
}

void MLFQ_push(void) {
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
     * core has a load greater or equal to current core's load. */
    if (target_mlfq == NULL || target_load >= MLFQ_load(current_mlfq)) {
        return;
    }

    foreach_level(current_mlfq) {
        MLFQ_level_t    *target_level   = NULL;

        target_level = MLFQ_get_level(target_mlfq, level - current_mlfq->level);
        assert(target_level, "Invalid target level.\n");

        if (!queue_trylock(&level->run_queue)) {
            continue;
        }

        if (!queue_trylock(&target_level->run_queue)) {
            // Locks not avalable.
            queue_unlock(&level->run_queue);
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

        if (err != 0) {
            debug("Error[%s]: Failed to migrate threads\n", strerror(err));
            queue_unlock(&level->run_queue);
            queue_unlock(&target_level->run_queue);
            continue;
        }

        count_pushed += count;

        queue_unlock(&level->run_queue);
        queue_unlock(&target_level->run_queue);
        // debug("load: %d, pushing: %d, pushed: %d to prio: %s\n",
        //     my_load, count, count_pushed, MLFQ_PRIORITY[target_level - target_mlfq->level]);
    }
}

void MLFQ_balance(void) {
    usize avg_load = 0, total = 0;
    foreach_MLFQ() {
        total += MLFQ_load(mlfq);
    }

    avg_load = total / ncpu();

    MLFQ_t *current_mlfq = MLFQ_get();
    usize current_load = MLFQ_load(current_mlfq);

    if (current_load > avg_load + 2) {
        MLFQ_push();
    } else if (current_load < avg_load - 2) {
        MLFQ_pull();
    }
}

static void MLFQ_aging(void) {
    foreach_MLFQ() {  // Iterate through ALL CPU MLFQs
        foreach_level_reverse(mlfq) {
            // Skip threads already in the highest priority
            if ((level - mlfq->level) == MLFQ_HIGH) {
                continue;
            }

            queue_t *source = &level->run_queue;
            queue_lock(source);

            thread_t *thread;
            queue_foreach_entry(source, thread, t_run_qnode) {
                thread_lock(thread);

                if (++thread->t_info.ti_sched.ts_age > AGING_THRESHOLD) {
                    int old_priority = 0, new_priority = 0;
                    thread_bump_priority(thread, THREAD_PRIO_INC, 1, &old_priority, &new_priority);

                    if (new_priority > MLFQ_HIGH) {
                        new_priority = MLFQ_HIGH;
                        thread_set_prio(thread, new_priority);
                    }

                    queue_t *target = &mlfq->level[new_priority].run_queue;

                    if (!queue_trylock(target)) {
                        thread_set_prio(thread, old_priority);
                        thread_unlock(thread);
                        continue;
                    }

                    int err = embedded_queue_relloc(
                        target, source, thread_node,
                        QUEUE_UNIQUE, QUEUE_TAIL
                    );

                    queue_unlock(target);

                    if (err != 0) {
                        thread_set_prio(thread, old_priority);
                        thread_unlock(thread);
                        continue;
                    }

                    thread->t_info.ti_sched.ts_age = 0;
                }
                thread_unlock(thread);
            }

            queue_unlock(source);
        }
    }
}

// For configuration parameters (if shared)
typedef struct {
    u64 aging_interval;
    u64 balance_interval;
}__aligned(64) sched_config; // Cache line aligned

__noreturn void scheduler_load_balancer(void) {
    static volatile u64 last_aging;
    static volatile u64 last_balance;

    volatile sched_config config = {
        .aging_interval = 100000000, // 100ms in nanoseconds
        .balance_interval = 1000000, // 1ms in nanoseconds
    };

    // Disable both preemption and migration to other CPUs.
    current_set(THREAD_NO_MIGRATE | THREAD_NO_PREEMPT);

    last_balance = last_aging = hpet_now();

    sigset_t set;
    sigsetfill(&set); // Block all signals except fatal ones
    sigsetdelmask(&set, SIGMASK(SIGKILL) | SIGMASK(SIGSTOP));
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    loop_and_yield() {
        const u64 now = hpet_now();

        if (time_before(now, last_aging)) last_aging = now;
        if (time_before(now, last_balance)) last_balance = now;

        // Aging: Run exactly every 100ms with drift compensation
        if (time_after(now, last_aging + config.aging_interval)) {
            bool intena = disable_interrupts();
            MLFQ_aging();
            enable_interrupts(intena);
            last_aging = now;
        }

        if (time_after(now, last_balance + config.balance_interval)) {
            MLFQ_balance();
            last_balance = now;
        }

        const u64 next_aging = last_aging + config.aging_interval;
        const u64 next_balance = last_balance + config.balance_interval;
        const u64 next_deadline = MIN(next_aging, next_balance);

        // Precision sleep using HPET until next needed action
        if (time_after(next_deadline, now)) {
            const u64 sleep_ns = next_deadline - now;
            hpet_nanosleep(sleep_ns);
            continue;
        }
    }
} BUILTIN_THREAD(scheduler_load_balancer, scheduler_load_balancer, NULL);