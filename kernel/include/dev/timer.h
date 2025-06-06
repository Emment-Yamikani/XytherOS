#pragma once

#include <dev/hpet.h>
#include <dev/rtc.h>
#include <core/types.h>

extern const ulong SYS_Hz;

extern void pit_init(void);
extern void pit_intr(void);
extern void pit_wait(double s);

#define time_after(unknown, known)        ((long)(known) - (long)(unknown) < 0)
#define time_before(unknown, known)       ((long)(unknown) - (long)(known) < 0)
#define time_after_eq(unknown, known)     ((long)(unknown) - (long)(known) >= 0)
#define time_before_eq(unknown, known)    ((long)(known) - (long)(unknown) >= 0)


#define ms_from_s(s)        ((double)(s) * 1000ul)        // convert seconds to ms
#define us_from_s(s)        ((double)(s) * 1000000ul)     // convert seconds to us
#define ns_from_s(s)        ((double)(s) * 1000000000ul)  // convert seconds to ns

#define s_from_ms(s)        ((double)(s) / 1000ul)        // convert ms to seconds
#define s_from_us(s)        ((double)(s) / 1000000ul)     // convert us to seconds
#define s_from_ns(s)        ((double)(s) / 1000000000ul)  // convert ns to seconds

#define s_from_HZ(Hz)       ((double)1 / (Hz))            // convert Hz to s
#define ms_from_HZ(Hz)      (ms_from_s(s_from_HZ(Hz)))    // convert Hz to ms
#define us_from_HZ(Hz)      (us_from_s(s_from_HZ(Hz)))    // convert Hz to us
#define ns_from_HZ(Hz)      (ns_from_s(s_from_HZ(Hz)))    // convert Hz to ns

#define NSEC_PER_USEC       (1000l)
#define NSEC_PER_MSEC       (1000000l)
#define NSEC_PER_SEC        (1000000000l)

#define USEC_PER_MSEC       (1000l)
#define USEC_PER_SEC        (1000000l)

#define MSEC_PER_SEC        (1000l)

#define JIFFIES_TO_TIMEVAL(jiffies, tv) ({       \
    do                                           \
    {                                            \
        ulong __secs = (jiffies) / SYS_Hz;       \
        ulong __usecs = ((jiffies) % SYS_Hz) *   \
                        (USEC_PER_SEC / SYS_Hz); \
        (tv)->tv_sec = __secs;                   \
        (tv)->tv_usec = __usecs;                 \
    } while (0);                                 \
})

#define JIFFIES_TO_TIMESPEC(jiffies, ts) ({      \
    do                                           \
    {                                            \
        ulong __secs = (jiffies) / SYS_Hz;       \
        ulong __nsecs = ((jiffies) % SYS_Hz) *   \
                        (NSEC_PER_SEC / SYS_Hz); \
        (ts)->tv_sec = __secs;                   \
        (ts)->tv_nsecs = __nsecs;                \
    } while (0);                                 \
})

#define TIMESPEC_TO_JIFFIES(ts) ({               \
    (((ts)->tv_sec * SYS_Hz) +                   \
     ((ts)->tv_nsec / (NSEC_PER_SEC / SYS_Hz))); \
})

#define TIMEVAL_TO_JIFFIES(tv) ({                \
    (((tv)->tv_sec * SYS_Hz) +                   \
     ((tv)->tv_usec / (USEC_PER_SEC / SYS_Hz))); \
})

#define jiffies_from_s(s)     ((jiffies_t)((double)SYS_Hz * (double)(s)))
#define jiffies_from_ms(ms)   (jiffies_from_s(s_from_ms(ms)))
#define jiffies_from_us(us)   (jiffies_from_s(s_from_us(us)))
#define jiffies_from_ns(ns)   (jiffies_from_s(s_from_ns(ns)))

#define s_from_jiffies(j)     ((double)(j) / SYS_Hz)
#define ms_from_jiffies(j)    (ms_from_s(s_from_jiffies(j)))
#define us_from_jiffies(j)    (us_from_s(s_from_jiffies(j)))
#define ns_from_jiffies(j)    (ns_from_s(s_from_jiffies(j)))

typedef ulong jiffies_t;

extern void jiffies_update(void);

extern jiffies_t jiffies_get(void);

extern void jiffies_timed_wait(jiffies_t jiffies);

extern int jiffies_sleep(jiffies_t jiffies, jiffies_t *rem);

extern int jiffies_getres(struct timespec *res);

extern int jiffies_gettime(struct timespec *tp);

extern void jiffies_to_timespec(jiffies_t jiffies, struct timespec *ts);

extern jiffies_t jiffies_from_timespec(const struct timespec *ts);

enum {
    TIMER_PIT   = 0,
    TIMER_RTC   = 1,
    TIMER_HPET  = 2,
};

extern int  timer_init(void);

extern void timer_intr(void);

extern void timer_wait(timer_t timer, double sec);

extern time_t epoch_get(void);

extern void epoch_update(void);

extern void epoch_set(time_t epoch);

extern void epoch_to_timespec(time_t epoch, struct timespec *ts);
extern time_t epoch_from_timespec(const struct timespec *ts);

extern time_t epoch_from_datetime(int year, int month, int day, int hour, int minute, int second);

extern void epoch_to_datetime(time_t epoch, int *year, int *month, int *day, int *hour, int *minute, int *second);