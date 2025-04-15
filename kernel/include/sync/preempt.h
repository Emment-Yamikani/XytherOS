#pragma once

#include <arch/cpu.h>
#include <arch/x86_64/asm.h>
#include <core/assert.h>

static inline bool pushcli(void) {
    bool intena = is_intena();
    cli();
    if (intena) cpu->intena = intena;
    cpu->ncli++;
    return intena;
}

static inline void popcli(void) {
    assert(!is_intena(), "Interrupts are enabled!\n");
    assert(--cpu->ncli >= 0, "!ncli: %d < 0\n", cpu->ncli);
    if (!cpu->ncli && cpu->intena) {
        cpu->intena = false;
        sti();
    }
}

extern void disable_preemption(void);
extern void enable_preemption(void);