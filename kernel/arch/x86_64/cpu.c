#include <arch/cpu.h>
#include <arch/firmware/acpi.h>
#include <arch/x86_64/asm.h>
#include <arch/x86_64/mmu.h>
#include <arch/x86_64/msr.h>
#include <arch/x86_64/lapic.h>
#include <bits/errno.h>
#include <boot/boot.h>
#include <core/debug.h>
#include <cpuid.h>
#include <mm/mem.h>
#include <string.h>
#include <sync/atomic.h>
#include <sync/preempt.h>
#include <sys/schedule.h>

static cpu_t *cpus[NCPU] = {NULL};
static volatile atomic_t cpus_count  = 1;
static volatile atomic_t cpus_online = 0;
static volatile atomic_t ap_continue = 0;

cpu_t *getcls(void) {
    return (cpu_t *)rdmsr(IA32_GS_BASE);
}

void setcls(cpu_t *c) {
    wrmsr(IA32_GS_BASE, (uintptr_t)c);
    wrmsr(IA32_KERNEL_GS_BASE, (uintptr_t)c);
}

thread_t *get_current(void) {
    disable_preemption();
    thread_t *thread = cpu ? cpu->thread : NULL;
    enable_preemption();
    return thread;
}

void set_current(thread_t *thread) {
    disable_preemption();
    cpu->thread = thread;
    enable_preemption();
}

int getcpuid(void) {
    u32 a = 0, b = 0, c = 0, d = 0;
    cpuid(0x1, 0, &a, &b, &c, &d);
    return ((b >> 24) & 0xFF);
}

void disable_preemption(void) {
    pushcli();
}
void enable_preemption(void) {
    popcli();
}

int cpu_online(void) {
    return atomic_read(&cpus_online);
}

int ncpu(void) {
    return (int)atomic_read(&cpus_count);
}

static void cpu_init(void) {
    idt_init();
    gdt_init();
    lapic_init();
    atomic_inc(&cpus_online);
    atomic_or(&cpu->flags, CPU_ONLINE);
    scheduler_init();
}

void bsp_init(void) {
    tvinit();
    cpu_init();
}

static void ap_start(void) {
    setcls(cpus[getcpuid()]);
    cpu_init();
    while (!atomic_read(&ap_continue)) {
        asm volatile ("pause");
    }
    loop() asm volatile ("pause");
}

void ap_signal(void) {
    atomic_write(&ap_continue, 1);
}

void bootothers(void) {
    extern u64 trampoline[PGSZ / sizeof(u64)];  // Trampoline code page

    /* Initialize BSP (Bootstrap Processor) entry */
    memset(cpus, 0, sizeof(cpus));
    cpus[getcpuid()] = cpu;

    /* Locate ACPI MADT table */
    acpiMADT_t *madt;
    assert((madt = (acpiMADT_t *)acpi_enumerate("APIC")),
        "Error: Failed to locate MADT table\n");

    /* Process MADT entries for APICs */
    for (char *entry = (char *)madt->apics; 
         entry < (char *)madt + madt->madt.length;
         entry += entry[1] /* Advance by entry length */) {
        
        if (*entry != ACPI_MADT_LAPIC) continue;

        typedef struct {            /* Local APIC Structure */
            u8  type;               /* Entry type (0)       */
            u8  len;                /* Entry length         */
            u8  acpi_id;            /* ACPI Processor ID    */
            u8  apic_id;            /* APIC ID              */
            u32 flags;              /* Processor flags      */
        } LapicEntry;

        LapicEntry *lapic = (LapicEntry *)entry;

        /* Skip current CPU and disabled entries */
        if (lapic->apic_id == getcpuid() || 
           !(lapic->flags & ACPI_LAPIC_ENABLED)) continue;

        /* Initialize CPU structure */
        assert((cpus[lapic->apic_id] = (cpu_t *)boot_alloc(sizeof(cpu_t), PGSZ)),
            "Failed to allocate CPU structure\n");
        
        memset(cpus[lapic->apic_id], 0, sizeof(cpu_t));
        cpus[lapic->apic_id]->flags |= CPU_ENABLED;
        cpus[lapic->apic_id]->apicID = lapic->apic_id;
        atomic_inc(&cpus_count);
    }

    /* Startup Application Processors */
    for (int i = 0; i < ncpu(); ++i) {
        if (!cpus[i] || cpus[i] == cpu || 
           !(cpus[i]->flags & CPU_ENABLED)) continue;

        /* Allocate AP boot stack */
        uintptr_t stack;
        assert((stack = (uintptr_t)boot_alloc(AP_STACK_SIZE, PGSZ)),
            "Failed to allocate AP stack\n");

        /* Configure trampoline data */
        enum TrampolineFields {
            TRAMPOLINE_PGTBL  = 4024 / sizeof(uintptr_t),  /* CR3 register  */
            TRAMPOLINE_STACK  = 4032 / sizeof(uintptr_t),  /* Stack pointer */
            TRAMPOLINE_ENTRY  = 4040 / sizeof(uintptr_t)   /* Entry point   */
        };

        trampoline[TRAMPOLINE_PGTBL] = rdcr3();               /* Page table  */
        trampoline[TRAMPOLINE_STACK] = stack + AP_STACK_SIZE; /* Stack top   */
        trampoline[TRAMPOLINE_ENTRY] = (uintptr_t)ap_start;   /* Entry point */

        /* Calculate SIPI vector from physical address */
        u16 vector = ((uintptr_t)trampoline);

        /* Start processor and wait for bringup */
        lapic_startup(cpus[i]->apicID, vector);
        while (!(atomic_read(&cpus[i]->flags) & CPU_ONLINE)) {
            asm volatile("pause");  /* Reduce bus contention */
        }
    }
}