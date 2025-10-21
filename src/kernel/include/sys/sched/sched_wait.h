#pragma once

#include <core/types.h>

typedef enum {
    WAKEUP_NONE     = 0,
    WAKEUP_NORMAL   = 1, // Normal wakeup.
    WAKEUP_SIGNAL   = 2, // Wakeup due to signal.
    WAKEUP_TIMEOUT  = 3, // Wakeup due to timeout.
    WAKEUP_ERROR    = 4, // Wakeup due to error.
} wakeup_t;

static inline int validate_wakeup_reason(wakeup_t reason) {
    return (reason != WAKEUP_NORMAL &&
            reason != WAKEUP_SIGNAL &&
            reason != WAKEUP_TIMEOUT) ? 0 : 1;
}