#pragma once

#include <core/types.h>
#include <arch/x86_64/mmu.h>

typedef struct {
    u64     flags;

    isize   ncli;
    bool    intena;

    idt_t   idt;
    gdt_t   gdt;
    tss_t   tss;
} cpu_t;

extern cpu_t *getcls(void);
extern void setcls(cpu_t *);

extern int getcpuid(void);

#define cpu     (getcls())