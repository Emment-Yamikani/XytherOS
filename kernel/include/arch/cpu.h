#pragma once

#include <arch/x86_64/mmu.h>
#include <core/types.h>

typedef struct cpu_t {
    u64         flags;

    isize       ncli;
    bool        intena;

    usize       timer_ticks;

    thread_t    *thread;

    gdt_t       gdt;
    tss_t       tss;
} cpu_t;

#define NCPU    8

extern cpu_t *getcls(void);
extern void setcls(cpu_t *);

extern int getcpuid(void);

extern int cpu_online(void);

#define cpu     (getcls())
#define current (cpu->thread)