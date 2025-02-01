#include <sys/thread.h>

tid_t gettid(void) {
    return current ? current->tid : 0;
}

pid_t getpid(void) {
    return current ? current->pid : 0;
}