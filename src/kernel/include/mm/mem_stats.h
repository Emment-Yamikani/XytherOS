#pragma once

typedef struct mem_stats {
    unsigned long used;
    unsigned long free; 
    unsigned long usable;
    unsigned long total;
} mem_stats_t;

int mem_get_stats(mem_stats_t *stats);