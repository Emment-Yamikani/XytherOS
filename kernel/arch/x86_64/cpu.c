#include <arch/cpu.h>
#include <arch/x86_64/asm.h>
#include <arch/x86_64/mmu.h>
#include <arch/x86_64/msr.h>
#include <cpuid.h>
#include <sync/atomic.h>

static volatile atomic_t n_online = 0;

cpu_t *getcls(void) {
    return (cpu_t *)rdmsr(IA32_GS_BASE);
}

void setcls(cpu_t *c) {
    wrmsr(IA32_GS_BASE, (uintptr_t)c);
    wrmsr(IA32_KERNEL_GS_BASE, (uintptr_t)c);
}

int getcpuid(void) {
    u32 a = 0, b = 0, c = 0, d = 0;
    cpuid(0x1, 0, &a, &b, &c, &d);
    return ((b >> 24) & 0xFF);
}

int cpu_online(void) {
    return atomic_read(&n_online);
}

int cpu_init(void) {
    cache_disable();
    tvinit();
    gdt_init();
    return 0;
}