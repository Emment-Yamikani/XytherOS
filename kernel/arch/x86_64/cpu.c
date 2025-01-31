#include <arch/cpu.h>
#include <arch/x86_64/mmu.h>
#include <arch/x86_64/msr.h>

cpu_t *getcls(void) {
    return (cpu_t *)rdmsr(IA32_GS_BASE);
}

void setcls(cpu_t *c) {
    wrmsr(IA32_GS_BASE, (uintptr_t)c);
    wrmsr(IA32_KERNEL_GS_BASE, (uintptr_t)c);
}

int cpu_init(void) {
    tvinit();
    gdt_init();
    return 0;
}