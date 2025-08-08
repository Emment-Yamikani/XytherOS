#include <arch/cpu.h>
#include <arch/firmware/acpi.h>
#include <arch/x86_64/asm.h>
#include <arch/x86_64/mmu.h>
#include <arch/x86_64/msr.h>
#include <arch/x86_64/lapic.h>
#include <bits/errno.h>
#include <boot/boot.h>
#include <core/debug.h>
#include <core/misc.h>
#include <cpuid.h>
#include <mm/mem.h>
#include <string.h>
#include <sync/atomic.h>
#include <sync/preempt.h>
#include <sys/schedule.h>
#include <dev/cga.h>

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

thread_t *cpu_getthread(void) {
    bool intena = disable_interrupts();
    thread_t *thread = cpu ? cpu->thread : NULL;
    enable_interrupts(intena);
    return thread;
}

bool cpu_set_thread(thread_t *thread) {
    bool intena = disable_interrupts();
    cpu->thread = thread;
    enable_interrupts(intena);
    return current ? true : false;
}

int getcpuid(void) {
    u32 a = 0, b = 0, c = 0, d = 0;
    cpuid(0x1, 0, &a, &b, &c, &d);
    return ((b >> 24) & 0xFF);
}

bool disable_interrupts(void) {
    bool intena = intrena();
    cli();
    return intena;
}

void enable_interrupts(bool intena) {
    if (intena) sti();
}

void cpu_set_ncli(isize ncli) {
    bool intr_st = disable_interrupts();
    swapi64(&cpu->ncli, &ncli);
    enable_interrupts(intr_st);
}

void cpu_set_intena(bool intena) {
    bool intr_st = disable_interrupts();
    swapbool(&cpu->intena, &intena);
    enable_interrupts(intr_st);
}

void cpu_set_preepmpt(isize ncli, bool intena) {
    bool intr_st = disable_interrupts();
    cpu->ncli    = ncli;
    cpu->intena  = intena;
    enable_interrupts(intr_st);
}

void cpu_swap_ncli(isize *ncli) {
    bool intr_st = disable_interrupts();
    swapi64(&cpu->ncli, ncli);
    enable_interrupts(intr_st);
}

void cpu_swap_intena(bool *intena) {
    bool intr_st = disable_interrupts();
    swapbool(&cpu->intena, intena);
    enable_interrupts(intr_st);
}

void cpu_swap_preempt(isize *ncli, bool *intena) {
    bool intr_st = disable_interrupts();
    swapi64(&cpu->ncli, ncli);
    swapbool(&cpu->intena, intena);
    enable_interrupts(intr_st);
}

isize cpu_get_ncli(void) {
    bool intena = disable_interrupts();
    isize value = cpu->ncli;
    enable_interrupts(intena);
    return value;
}

bool cpu_get_intena(void) {
    bool intena = disable_interrupts();
    bool value  = cpu->intena;
    enable_interrupts(intena);
    return value;
}

int cpu_online(void) {
    return atomic_read(&cpus_online);
}

int ncpu(void) {
    return (int)atomic_read(&cpus_count);
}

bool isbsp(void) {
    bool intena = disable_interrupts();
    bool bsp = (atomic_read(&cpu->flags) & CPU_BSP) ? true : false;
    enable_interrupts(intena);
    return bsp;
}

static void cpu_init(void) {
    idt_init();
    gdt_init();
    lapic_init();
    scheduler_init();
    atomic_inc(&cpus_online);
    atomic_or(&cpu->flags, CPU_ONLINE);
}

int cpu_rsel(void) {
    static atomic_t i = 0;
    return (atomic_inc(&i) % ncpu());
}

void bsp_init(void) {
    atomic_or(&cpu->flags, CPU_BSP);
    tvinit();
    cpu_init();
}

static void ap_start(void) {
    setcls(cpus[getcpuid()]);
    cpu_init();
    while (!atomic_read(&ap_continue)) {
        cpu_pause();
    }
    scheduler();
    loop() cpu_pause();
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
           !(cpus[i]->flags & CPU_ENABLED)) {
            continue;
        }

        /* Allocate AP boot stack */
        const uintptr_t stack = (uintptr_t)boot_alloc(AP_STACK_SIZE, PGSZ);
        assert_ne(stack, NULL, "Failed to allocate AP stack\n");

        /* Configure trampoline data */
        enum TrampolineFields {
            TRAMPOLINE_PGTBL  = 4024 / sizeof(uintptr_t),  /* CR3 register  */
            TRAMPOLINE_STACK  = 4032 / sizeof(uintptr_t),  /* Stack pointer */
            TRAMPOLINE_ENTRY  = 4040 / sizeof(uintptr_t),  /* Entry point   */
        };

        trampoline[TRAMPOLINE_PGTBL] = rdcr3();               /* Page table  */
        trampoline[TRAMPOLINE_STACK] = stack + AP_STACK_SIZE; /* Stack top   */
        trampoline[TRAMPOLINE_ENTRY] = (uintptr_t)ap_start;   /* Entry point */

        /* Calculate SIPI vector from physical address */
        u16 vector = ((uintptr_t)trampoline);

        /* Start processor and wait for bringup */
        lapic_startup(cpus[i]->apicID, vector);
        while (!(atomic_read(&cpus[i]->flags) & CPU_ONLINE)) {
            cpu_pause();  /* Reduce bus contention */
        }
    }
}