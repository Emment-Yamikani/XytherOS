#include <sync/preempt.h>
#include <sys/thread.h>

void disable_preemption(void) {
    if (current) {
        current_set_no_prempt();
    }
}

void enable_preemption(void) {
    if (current) {
        current_unset_no_prempt();
    }
}