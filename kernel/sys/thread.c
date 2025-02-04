#include <sys/thread.h>

tid_t gettid(void) {
    return current ? current->t_info.ti_tid : 0;
}

pid_t getpid(void) {
    return current ? current->t_pid : 0;
}
