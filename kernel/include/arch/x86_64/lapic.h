#pragma once

#include <core/types.h>

#define IPI_ALL         (-2) // broadcast to all.
#define IPI_SELF        (-1) // send to self.
#define IPI_ALLXSELF    (-3) // broadcast to all except self.

#define IPI_SYNC        0
#define IPI_TLBSHTDWN   1

extern int lapic_id(void);
extern int lapic_init(void);
extern void lapic_eoi(void);
extern void lapic_timerintr(void);
// extern void lapic_setaddr(uintptr_t);
void lapic_send_ipi(int ipi, int dst);
void lapic_recalibrate(long hz);
extern void lapic_startup(int id, u16 addr);