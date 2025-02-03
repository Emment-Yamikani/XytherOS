#pragma once

#include <arch/x86_64/mmu.h>
#include <core/types.h>

typedef struct cpu_t {
    int         apicID;

    u64         flags;

    isize       ncli;
    bool        intena;

    usize       timer_ticks;

    thread_t    *thread;

    gdt_t       gdt;
    tss_t       tss;
} cpu_t;

#define NCPU    8

#define CPU_ENABLED 0x1
#define CPU_ONLINE  0x2

#define AP_STACK_SIZE   0x4000

extern cpu_t *getcls(void);
extern void setcls(cpu_t *);

extern int getcpuid(void);

extern int cpu_online(void);

extern int ncpu(void);
extern void ap_signal(void);

#define cpu     (getcls())
#define current (cpu->thread)