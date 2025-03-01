#pragma once

#include <arch/x86_64/mmu.h>
#include <core/types.h>

typedef struct cpu_t {
    int         apicID;

    u64         flags;

    isize       ncli;
    isize       intena;

    usize       timer_ticks;

    thread_t    *thread;

    gdt_t       gdt;
    tss_t       tss;
} cpu_t;

#define NCPU    8

#define CPU_ENABLED     0x1
#define CPU_ONLINE      0x2
#define CPU_BSP         0x4

#define AP_STACK_SIZE   0x4000

// acce
extern cpu_t *getcls(void);
extern void setcls(cpu_t *);

extern int getcpuid(void);

extern int cpu_online(void);

extern int cpu_rsel(void);
extern int ncpu(void);
extern void ap_signal(void);

extern int isbsp(void);

// access with interrupts disabled.
#define cpu     (getcls())

extern thread_t *get_current(void);
extern void set_current(thread_t *thread);

extern void disable_preemption(void);
extern void enable_preemption(void);

extern void cpu_swap_preepmpt(isize *ncli, isize *intena);

#define current (get_current())
