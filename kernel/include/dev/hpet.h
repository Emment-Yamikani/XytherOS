#pragma once

#include <core/types.h>
#include <sync/spinlock.h>

typedef struct {
    int      t_id;     // timer ID.
    unsigned t_flags;  // timer operation flags.
    uint64_t t_value;  // initial counter value.
} hpet_timer_t;

extern int      hpet_init(void);
extern void     hpet_intr(void);
extern long     hpet_freq(void);
extern void     hpet_wait(double s);
extern size_t   hpet_rdmcnt(void);