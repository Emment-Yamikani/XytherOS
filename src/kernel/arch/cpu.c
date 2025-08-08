#include <arch/cpu.h>
#include <bits/errno.h>
#include <sync/atomic.h>

usize cpu_getticks(void) {
    bool intena = disable_interrupts();
    usize ticks = cpu->timer_ticks;
    enable_interrupts(intena);
    return ticks;
}

void cpu_upticks(void) {
    bool intena = disable_interrupts();
    atomic_inc(&cpu->timer_ticks);
    enable_interrupts(intena);
}


cpu_flags_t cpu_getflags(void) {
    bool intena = disable_interrupts();
    cpu_flags_t flags = atomic_read(&cpu->flags);
    enable_interrupts(intena);
    return flags;
}

cpu_flags_t cpu_setflags(cpu_flags_t flags) {
    bool intena = disable_interrupts();
    cpu_flags_t old_flags = atomic_fetch_or(&cpu->flags, flags);
    enable_interrupts(intena);
    return old_flags;
}

cpu_flags_t cpu_testflags(cpu_flags_t flags) {
    bool intena = disable_interrupts();
    cpu_flags_t cpu_flags = atomic_read(&cpu->flags);
    enable_interrupts(intena);
    return cpu_flags & flags;
}

cpu_flags_t cpu_maskflags(cpu_flags_t flags) {
    bool intena = disable_interrupts();
    cpu_flags_t old_flags = atomic_fetch_and(&cpu->flags, ~flags);
    enable_interrupts(intena);
    return old_flags;
}