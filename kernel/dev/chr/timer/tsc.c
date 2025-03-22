#include <arch/x86_64/asm.h>
#include <cpuid.h>
#include <dev/tsc.h>
#include <lib/printk.h>

bool tsc_is_invariant(void) {
    uint reg[4] = {0};
    __get_cpuid(0x80000007, &reg[0], &reg[1], &reg[2], &reg[3]);
    return (reg[3] & (1 << 8)) ? true : false;
}

time_t tsc_get(void) {
    return (time_t) rdtsc();
}

void tsc_timed_wait(time_t time);

int tsc_sleep(time_t time, time_t *rem);

int tsc_getres(struct timespec *res);

int tsc_gettime(struct timespec *tp);

void tsc_to_timespec(time_t time, struct timespec *ts);

time_t tsc_from_timespec(const struct timespec *ts);