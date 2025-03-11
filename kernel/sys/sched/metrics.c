#include "metrics.h"
#include <core/debug.h>

void sched_update_thread_metrics(thread_t *thread) {
    thread_sched_t *sched_info = NULL;

    thread_assert_locked(thread);

    sched_info = &thread->t_info.ti_sched;

    switch (thread_get_state(thread)) {
    case T_RUNNING:
        sched_info->ts_sched_count      += 1;
        sched_info->ts_last_time        = epoch_get();
        sched_info->ts_last_timeslice   = sched_info->ts_timeslice;
        break;
    case T_ZOMBIE:
    case T_TERMINATED:
        sched_info->ts_exit_time = epoch_get();
        __fallthrough;
    case T_READY:
    case T_SLEEP:
    case T_STOPPED:
        sched_info->ts_cpu_time     += sched_info->ts_last_timeslice - sched_info->ts_timeslice;
        sched_info->ts_cpu_time     += jiffies_from_s(sched_info->ts_last_time - epoch_get());
        sched_info->ts_total_time   += sched_info->ts_cpu_time;
        break;
    default:
        todo("Handle state[%s]\n", tget_state(thread_get_state(thread)));
    }
}