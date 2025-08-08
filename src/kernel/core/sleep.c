#include <bits/errno.h>
#include <core/debug.h>
#include <core/timer.h>
#include <mm/kalloc.h>
#include <string.h>
#include <sys/schedule.h>

int nanosleep(const timespec_t *duration, timespec_t *rem) {
    jiffies_t   timeout, remainder;
    
    if (duration == NULL) {
        return -EFAULT;
    }
    
    if ((duration->tv_nsec > 999999999l) || (duration->tv_sec < 0)) {
        return -EINVAL;
    }
    
    timeout = jiffies_from_timespec(duration);
    int err = jiffies_sleep(timeout, &remainder);
    if (rem != NULL) {
        jiffies_to_timespec(remainder, rem);
    }
    return err;
}