#pragma once

#include <arch/x86_64/mmu.h>
#include <core/types.h>

typedef u64 cpu_flags_t;

typedef struct cpu_t {
    int         apicID;

    cpu_flags_t flags;

    isize       ncli;
    bool        intena;

    usize       timer_ticks;

    thread_t    *thread;

    gdt_t       gdt;
    descptr_t   gdtptr;
    tss_t       tss;
    descptr_t   idtptr;
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

extern bool isbsp(void);

// Callers **MUST** access with interrupts disabled.
#define cpu     (getcls())

extern isize cpu_get_ncli(void);
extern bool cpu_get_intena(void);

extern thread_t *cpu_getthread(void);
extern bool cpu_set_thread(thread_t *thread);

extern bool disable_interrupts(void);
extern void enable_interrupts(bool old_int_status);

extern void cpu_set_ncli(isize ncli);

extern void cpu_set_intena(bool intena);

extern void cpu_set_preepmpt(isize ncli, bool intena);

extern void cpu_swap_preempt(isize *ncli, bool *intena);

extern void cpu_swap_ncli(isize *ncli);

extern void cpu_swap_intena(bool *intena);

#define current (cpu_getthread())

extern usize cpu_getticks(void);
extern void  cpu_upticks(void);

extern cpu_flags_t cpu_getflags(void);
extern cpu_flags_t cpu_setflags(cpu_flags_t flags);
extern cpu_flags_t cpu_testflags(cpu_flags_t flags);
extern cpu_flags_t cpu_maskflags(cpu_flags_t flags);

extern void nm_except(void);
extern void mf_except(void);
extern void smid_except(void);