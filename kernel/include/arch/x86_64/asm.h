#pragma once

#include <core/types.h>

extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);
extern void isr128(void);

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);
extern void irq16(void);
extern void irq17(void);
extern void irq18(void);
extern void irq19(void);
extern void irq20(void);
extern void irq21(void);
extern void irq22(void);
extern void irq23(void);
extern void irq24(void);
extern void irq25(void);
extern void irq26(void);
extern void irq27(void);
extern void irq28(void);
extern void irq29(void);
extern void irq30(void);
extern void irq31(void);
extern void irq32(void);
extern void irq33(void);

extern u64 rdtsc(void);
extern u64 rdtscp(int *aux);

extern u64 rdrax(void);

extern u64 rdrfl(void);

static inline void hlt(void) { asm volatile ("hlt" ::: "memory"); }

static inline void cpu_pause(void) { asm volatile ("pause" ::: "memory"); }

static inline void cli(void) { asm volatile ("cli" ::: "memory"); }

static inline void sti(void) { asm volatile ("sti" ::: "memory"); }

static inline bool intrena(void) { return (rdrfl() & 0x200) ? true : false; }

extern void wrrfl(u64);

extern u64 rdcr0(void);

extern void wrcr0(u64);

extern u64 rdcr2(void);

extern u64 rdcr3(void);

extern void wrcr3(u64);

extern u64 rdcr4(void);

extern void wrcr4(u64);

extern void cache_disable(void);

extern void loadgdt64();
extern void loadidt();
extern void loadtr();

extern void invlpg(uintptr_t);

extern void wrxcr(u64);

extern u64 rdxcr(u64);

extern void fxsave(u64);

extern void fxrstor(u64);

extern u64 rdrsp(void);

extern u64 rdrbp(void);

extern void fninit(void);

extern bool CmpXchg(volatile u64 *ptr, u64 expected, u64 new_value);

extern u8 inb(u16 port);
extern void outb(u16 port, u8 data);

extern u16 inw(u16 port);
extern void outw(u16 port, u16 data);

extern u32 ind(u16 port);
extern void outd(u16 port, u32 data);
