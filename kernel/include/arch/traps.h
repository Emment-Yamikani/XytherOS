#pragma once

#define IRQ_OFFSET      32
#define IRQ(i)          (IRQ_OFFSET + (i))

#define ISR_OFFSET      0
#define ISR(i)          (ISR_OFFSET + (i))

#define T_DE            ISR(0)  // Divide error.
#define T_DB            ISR(1)  // Debug exception.
#define T_NMI           ISR(2)  // Non-maskable interrupt.
#define T_BP            ISR(3)  // Breakpoint.
#define T_OF            ISR(4)  // Overflow.
#define T_BR            ISR(5)  // Bound range exceeded.
#define T_UD            ISR(6)  // Undefined Opcode.
#define T_NM            ISR(7)  // No Math Coprocessor.
#define T_DF            ISR(8)  // Double fault.
#define T_CSO           ISR(9)  // Coprocessor overrun.
#define T_TS            ISR(10) // Invalid TSS.
#define T_NP            ISR(11) // Segment Not Present.
#define T_SS            ISR(12) // Stack Segment Fault.
#define T_GP            ISR(13) // General Protection Fault.
#define T_PF            ISR(14) // Page Fault.
#define T_MF            ISR(16) // x87 FPU Math Fault.
#define T_AC            ISR(17) // Alignment Check.
#define T_MC            ISR(18) // Machine Check.
#define T_XM            ISR(19) // SIMD Floating-Point Exception
#define T_VE            ISR(20) // Virtualization Exception.

#define T_PIT            IRQ(0) // PIT interrupt line.
#define T_PS2_KBD        IRQ(1) // PS2 KBD interrupt line.
#define T_HPET           IRQ(2) // HPET interrupt line.
#define T_SPURIOUS       IRQ(7) // Spurious interrupt.
#define T_RTC            IRQ(8) // RTC interrupt line.
#define T_LAPIC_ERROR    IRQ(18)// LAPIC error interrupt line.
#define T_LAPIC_SPURIOUS IRQ(19)// LAPIC spurious interrupt line.
#define T_LAPIC_TIMER    IRQ(20)// LAPIC timer interrupt line.
#define T_TLBSHTDWN      IRQ(31)// TLB shootdown interrupt.
#define T_LAPIC_IPI      IRQ(32)// LAPIC Interprocessor interrupt(IPI).
#define T_PANIC          IRQ(33)// PANIC IPI.

#define T_LEG_SYSCALL    IRQ(96) // Legacy system call entry.
#define T_SIMTRAP        IRQ(128)// Trap simulation.