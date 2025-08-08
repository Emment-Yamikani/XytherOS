#pragma once

extern void pic_enable(int irq);
extern int interrupt_controller_init(void);
extern void enable_intr_line(int irq, int core_id);

extern void pit_init(void);
extern void pit_intr(void);
extern void pit_wait(double s);