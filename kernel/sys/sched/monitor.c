#include "metrics.h"
#include <dev/cga.h>

typedef struct {
    uint64_t prev_active;
    uint64_t prev_idle;
} util_prev_t;

const int BAR_LENGTH    = 10;

#define COLOR_DGREEN    "\e[32m"
#define COLOR_GREEN     "\e[92m"
#define COLOR_YELLOW    "\e[96m"
#define COLOR_DYELLOW   "\e[36m"
#define COLOR_RED       "\e[94m"
#define COLOR_DRED      "\e[34m"

#define COLOR_RESET     "\e[0m"

void print_bar(const char *label, int x, int y, int percent, int load) {
    char bars[101] = {0};
    int  pos = 0, nbars = (percent * BAR_LENGTH) / 100;
    
    percent = percent >= 100 ? 100 : percent;

    for (int i = 0; i < BAR_LENGTH; i++) {
        const char *color;
        switch (i) {
            case 0 ... 1: color = COLOR_DGREEN; break;
            case 2 ... 3: color = COLOR_GREEN; break;
            case 4 ... 5: color = COLOR_YELLOW; break;
            case 6 ... 7: color = COLOR_DYELLOW; break;
            case 8 ... 9: color = COLOR_DRED; break;
            case /*.*/10: color = COLOR_RED; break;
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

    printk("\e[s\e[%d;%dH\e[0K%s [%s] %3d%% Load: %3d threads\e[u",
           y, x, label, bars, percent, load);
}

static usize avg_percent = 0;
static usize total_load = 0;

void update_utilization(util_prev_t *prev_stats) {
    for (int core = 0; core < ncpu(); core++) {
        sched_metrics_t *m = get_cpu_metrics(core);

        // Get current values
        const usize curr_active = atomic_read(&m->last_active_time);
        const usize curr_idle = atomic_read(&m->last_idle_time);
        const usize load = atomic_read(&m->load) + (atomic_read(&m->idle) ? 0 : 1);

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

        char label[8] = {0};
        snprintf(label, sizeof label, "CPU%d", core);

        // Print utilization bar
        print_bar(label, 0, 25 - ncpu() + core, percent, load); 
    }

    avg_percent /= ncpu();
    print_bar("AVRG", 0, 25, avg_percent, total_load);
    avg_percent = total_load = 0;
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
        hpet_milliwait(1000);
        update_utilization(prev_stats);
    }
} BUILTIN_THREAD(utilization_monitor, utilization_monitor, NULL);