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
extern void     hpet_microwait(ulong us);
extern void     hpet_milliwait(ulong ms);
extern void     hpet_nanosleep(ulong ns);
extern ulong    hpet_now(void);

extern int hpet_getres(struct timespec *res);
extern int hpet_gettime(struct timespec *tp);
extern void hpet_to_timespec(ulong time, struct timespec *ts);
extern ulong hpet_from_timespec(const struct timespec *ts);