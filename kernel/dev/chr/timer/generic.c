#include <bits/errno.h>
#include <core/debug.h>
#include <core/timer.h>
#include <dev/timer.h>
#include <sync/atomic.h>
#include <sys/schedule.h>
#include <sys/_time.h>

const ulong SYS_Hz = 1000;

static volatile bool use_hpet    = 0;

void timer_wait(timer_t timer, double sec) {
    switch (timer) {
        case TIMER_PIT: pit_wait(sec); break;
        case TIMER_RTC:
        case TIMER_HPET: hpet_wait(sec); break;
        break;
    }
}

int timer_init(void) {
    int err = 0;
    if ((err = hpet_init())) {
        debug("Error[%s]: HPET init failed. Fallback to PIT\n", perror(err));
        pit_init();
        return 0;
    }

    atomic_write(&use_hpet, true);
    return 0;
}

void timer_intr(void) {
    if (use_hpet)
        hpet_intr();
    else pit_intr();
}