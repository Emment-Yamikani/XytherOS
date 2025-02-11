#pragma once

#include <core/types.h>
#include <sync/spinlock.h>

typedef struct {
    int      t_id;     // timer ID.
    unsigned t_flags;  // timer operation flags.
    uint64_t t_value;  // initial counter value.
} hpet_timer_t;


#define HPET_TMR_32             BS(0)
#define HPET_TMR_PER            BS(1)
#define HPET_TMR_LVL            BS(2)

#define HPET_TMR_IS32(flags)    BTEST(flags, 0)
#define HPET_TMR_ISPER(flags)   BTEST(flags, 1)
#define HPET_TMR_ISLVL(flags)   BTEST(flags, 2)

extern int      hpet_init(void);
extern void     hpet_intr(void);
extern long     hpet_freq(void);
extern void     hpet_wait(double s);
extern size_t   hpet_rdmcnt(void);