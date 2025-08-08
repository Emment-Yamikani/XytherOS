#pragma once

#include <arch/cpu.h>
#include <arch/x86_64/asm.h>
#include <core/assert.h>

static inline bool pushcli(void) {
    bool intena = disable_interrupts();
    if (intena) cpu->intena = intena;
    cpu->ncli++;
    return intena;
}

static inline void popcli(void) {
    assert(!intrena(), "Interrupts are enabled!\n");
    assert(--cpu->ncli >= 0, "!ncli: %d < 0\n", cpu->ncli);

    bool intena = cpu->intena;
    if (!cpu->ncli && intena) {
        cpu->intena = false;
        enable_interrupts(intena);
    }
}

extern void disable_preemption(void);
extern void enable_preemption(void);