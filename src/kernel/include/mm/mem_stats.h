#pragma once

typedef struct mem_stats {
    unsigned long used;
    unsigned long free; 
    unsigned long usable;
    unsigned long total;
} mem_stats_t;

int getmemstats(mem_stats_t *stats);