#include <bits/errno.h>
#include <sys/_time.h>
#include <core/defs.h>
#include <dev/rtc.h>
#include <core/types.h>



int gettimeofday(struct timeval *restrict tp, void *restrict tzp __unused) {
    time_t time = rtc_gettime();
    tp->tv_usec = 0;
    tp->tv_sec  = time;
    return 0;
}

int settimeofday(const struct timeval *tv __unused, const struct timezone *tz __unused) {
    return -ENOTSUP;
}