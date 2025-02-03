#include <dev/timer.h>

void timer_wait(timerid_t timer, double sec) {
    switch (timer) {
        case TIMER_PIT: pit_wait(sec); break;
        case TIMER_RTC:
        case TIMER_HPET:
        break;
    }
}