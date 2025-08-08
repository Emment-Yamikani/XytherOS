#include "metrics.h"
#include <boot/boot.h>
#include <dev/cga.h>
#include <mm/mem.h>
#include <sync/atomic.h>

static atomic_u8 monitor_active = false;

typedef struct {
    uint64_t prev_active;
    uint64_t prev_idle;
} util_prev_t;

const int BAR_LENGTH    = 10;

#define COLOR_DGREEN    "\e[32m"
#define COLOR_GREEN     "\e[92m"
#define COLOR_BLUE      "\e[91m"
#define COLOR_CYAN      "\e[93m"
#define COLOR_DMAGENTA  "\e[35m"
#define COLOR_MAGENTA   "\e[95m"
#define COLOR_RED       "\e[94m"
#define COLOR_DRED      "\e[34m"

#define COLOR_RESET     "\e[0m"


static inline int process_usage_meter(char *bars, int pos, int percent) {
    percent = percent >= 100 ? 100 : percent;
    int nbars = (percent * BAR_LENGTH) / 100;

    for (int i = 0; i < BAR_LENGTH; i++) {
        const char *color;
        switch (i) {
            case 0 ... 1: color = COLOR_DGREEN; break;
            case 2 ... 3: color = COLOR_GREEN;  break;
            case /*..*/4: color = COLOR_CYAN;   break;
            case /*..*/5: color = COLOR_BLUE;   break;
            case /*..*/6: color = COLOR_DMAGENTA; break;
            case /*..*/7: color = COLOR_MAGENTA; break;
            case /*..*/8: color = COLOR_RED;    break;
            case 9 ... 10: color = COLOR_DRED;   break;
        }

        // Copy color code (5 bytes)
        memcpy(bars + pos, color, 5);
        pos += 5;

        // Add bar character
        bars[pos++] = i < nbars ? '|' : ' ';
        
        // Copy reset code (4 bytes)
        memcpy(bars + pos, COLOR_RESET, 4);
        pos += 4;
    }

    bars[pos] = '\0';
    return pos;
}

static inline void memory_bar(char *bars) {
    float mm_used_sz = (float)pmman.mem_used() / 1024.;
    float mm_total   = (float)bootinfo.total / 1024.;
    float percent    = (mm_used_sz * 100) / mm_total;

    memcpy(bars, "| MEM [", 8);
    int pos = process_usage_meter(bars, 7, percent);

    pos += sprintf(bars + pos, "%d%%] %2.1f/%2.0fMiB", (int)percent, mm_used_sz, mm_total);
    bars[pos] = '\0';
}

static usize avg_percent = 0;
static usize total_load = 0;

void update_utilization(util_prev_t *prev_stats) {
    for (int core = 0; core < ncpu(); core++) {
        sched_metrics_t *metrics = get_cpu_metrics(core);

        // Get current values
        const usize curr_active = atomic_read(&metrics->last_active_time);
        const usize curr_idle = atomic_read(&metrics->last_idle_time);
        const usize load = atomic_read(&metrics->load) + (atomic_read(&metrics->idle) ? 0 : 1);

        // Calculate deltas
        uint64_t delta_active = curr_active - prev_stats[core].prev_active;
        uint64_t delta_idle = curr_idle - prev_stats[core].prev_idle;
        uint64_t total = delta_active + delta_idle;

        // Update previous values
        prev_stats[core].prev_active = curr_active;
        prev_stats[core].prev_idle = curr_idle;

        // Calculate percentage
        int percent = total ? (delta_active * 100) / total : 0;

        avg_percent += percent;
        total_load  += load;

        
        // Print utilization bar
        char bars[101];
        process_usage_meter(bars, 0, percent);
        printk("\e[s\e[%d;%dHCPU%d [%s %3d%%] %3d thr\e[0K\e[u",
                25 - ncpu() + core, 0, core, bars, percent, load);
    }

    char mem_meter[150];
    memory_bar(mem_meter);

    avg_percent /= ncpu();

    char cpu_meter[150];
    process_usage_meter(cpu_meter, 0, avg_percent);
    printk("\e[s\e[%d;%dHCPU  [%s %3d%%] %3d thr %s\e[0K\e[u",
            25, 0, cpu_meter, avg_percent, total_load, mem_meter);
    avg_percent = total_load = 0;
}

void toggle_sched_monitor(void) {
    atomic_xor(&monitor_active, true);
}

// Utilization monitor thread
__noreturn void utilization_monitor() {
    util_prev_t prev_stats[ncpu()];

    // Initialize previous values
    for (int core = 0; core < ncpu(); core++) {
        sched_metrics_t *metrics = get_cpu_metrics(core);
        prev_stats[core].prev_active = atomic_read(&metrics->last_active_time);
        prev_stats[core].prev_idle = atomic_read(&metrics->last_idle_time);
    }

    loop_and_yield() {
        if (!monitor_active) {
            sched_yield();
            continue;
        }

        hpet_milliwait(1000);
        update_utilization(prev_stats);
    }
} BUILTIN_THREAD(utilization_monitor, utilization_monitor, NULL);