#include <bits/errno.h>
#include <core/debug.h>
#include <sys/schedule.h>
#include <sys/thread.h>

int sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout);