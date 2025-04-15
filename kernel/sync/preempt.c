#include <sync/preempt.h>
#include <sys/thread.h>

void disable_preemption(void) {
    if (current) {
        bool locked = current_recursive_lock();
        current->t_info.ti_flags |= THREAD_NO_PREEMPT;
        if (locked) current_unlock();
    }
}

void enable_preemption(void) {
    if (current) {
        bool locked = current_recursive_lock();
        current->t_info.ti_flags &= ~THREAD_NO_PREEMPT;
        if (locked) current_unlock();
    }
}