#include <dev/timer.h>
#include <sys/_time.h>

static volatile atomic_t epoch_time = 0;

static const    int   days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

u64 epoch_get(void) {
    return atomic_read(&epoch_time);
}

void epoch_set(u64 epoch) {
    atomic_write(&epoch_time, epoch);
}

void epoch_update(void) {
    atomic_inc(&epoch_time);
}

static int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

void epoch_to_datetime(u64 epoch, int *year, int *month, int *day, int *hour, int *minute, int *second) {
    int days    = epoch / 86400;  // Total days since 1970

    *second     = epoch % 60;
    *minute     = (epoch / 60) % 60;
    *hour       = (epoch / 3600) % 24;

    // Calculate the year
    *year = 1970;
    while (days >= (is_leap_year(*year) ? 366 : 365)) {
        days -= is_leap_year(*year) ? 366 : 365;
        (*year)++;
    }

    // Calculate the month and day
    *month = 0;
    while (days >= days_in_month[*month] + ((*month == 1 && is_leap_year(*year)) ? 1 : 0)) {
        days -= days_in_month[*month] + ((*month == 1 && is_leap_year(*year)) ? 1 : 0);
        (*month)++;
    }
    *day = days + 1;
}

u64 epoch_from_datetime(int year, int month, int day, int hour, int minute, int second) {
    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31)
        return 0; // Invalid date

    u64 epoch = 0;

    // Add days from past years
    for (int y = 1970; y < year; y++) {
        epoch += is_leap_year(y) ? 366 : 365;
    }

    // Add days from past months in the current year
    for (int m = 0; m < month - 1; m++) {
        epoch += days_in_month[m];
        if (m == 1 && is_leap_year(year)) // February in a leap year
            epoch++;
    }

    // Add days from current month, converting everything to seconds
    epoch = (epoch + (day - 1)) * 86400; // Convert days to seconds
    epoch += hour * 3600;                // Convert hours to seconds
    epoch += minute * 60;                // Convert minutes to seconds
    epoch += second;                     // Add remaining seconds

    return epoch;
}

void epoch_to_timespec(uint64_t epoch, struct timespec *ts) {
    ts->tv_sec = epoch;
    ts->tv_nsec = 0;
}

uint64_t epoch_from_timespec(const struct timespec *ts) {
    return ts->tv_sec;
}
