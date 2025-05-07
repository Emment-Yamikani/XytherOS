#include "metrics.h"
#include <core/debug.h>

sched_metrics_t per_cpu_metrics[NCPU];

sched_metrics_t *get_cpu_metrics(int core) {
    return &per_cpu_metrics[core];
}

sched_metrics_t *get_metrics(void) {
    return &per_cpu_metrics[getcpuid()];
}

void sched_update_thread_metrics(thread_t *thread) {
    thread_assert_locked(thread);

    thread_sched_t *ts = &thread->t_info.ti_sched;
    int pr = ts->ts_prio;

    // clamp thread priority.
    ts->ts_prio = pr > MLFQ_HIGH ? MLFQ_HIGH : pr < 0 ? 0 : pr;

    switch (thread_get_state(thread)) {
    case T_RUNNING:
        ts->ts_sched_count      += 1;
        ts->ts_last_time        = epoch_get();
        ts->ts_last_timeslice   = ts->ts_timeslice;
        break;
    case T_ZOMBIE:
    case T_TERMINATED:
        ts->ts_exit_time = epoch_get();
        __fallthrough;
    case T_READY:
    case T_SLEEP:
    case T_STOPPED:
        ts->ts_cpu_time     += ts->ts_last_timeslice - ts->ts_timeslice;
        ts->ts_cpu_time     += jiffies_from_s(ts->ts_last_time - epoch_get());
        ts->ts_total_time   += ts->ts_cpu_time;
        break;
    default:
        todo("Handle state[%s]\n", tget_state(thread_get_state(thread)));
    }
}