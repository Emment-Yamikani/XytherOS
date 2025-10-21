#include <bits/errno.h>
#include <boot/boot.h>
#include <core/defs.h>
#include <mm/mem_stats.h>
#include <mm/mem.h>

int getmemstats(mem_stats_t *stats) {
    if (stats == NULL) {
        return -EINVAL;
    }

    stats->total    = bootinfo.total;
    stats->usable   = bootinfo.usable;
    stats->used     = pmman.mem_used();
    stats->free     = stats->total - stats->used;

    return 0;
}