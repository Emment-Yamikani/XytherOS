#include <dev/timer.h>
#include <sync/atomic.h>

static volatile jiffies_t jiffies = 0; 

void jiffies_update(void) {
    atomic_inc(&jiffies);
}

jiffies_t jiffies_get(void) {
    return atomic_read(&jiffies);
}

void jiffies_timed_wait(double s);

jiffies_t jiffies_sleep(jiffies_t jiffies);

int jiffies_getres(struct timespec *res);

int jiffies_gettime(struct timespec *tp);

int jiffies_settime(const struct timespec *tp);

void timer_wait(timerid_t timer, double sec) {
    switch (timer) {
        case TIMER_PIT: pit_wait(sec); break;
        case TIMER_RTC:
        case TIMER_HPET:
        break;
    }
}
