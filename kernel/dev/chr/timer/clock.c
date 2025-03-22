#include <bits/errno.h>
#include <core/debug.h>
#include <core/posix_timer.h>

int clock_getres(clockid_t clockid, struct timespec *res);
int clock_gettime(clockid_t clockid, struct timespec *tp);
int clock_settime(clockid_t clockid, const struct timespec *tp);
