#include <core/debug.h>
#include <dev/dev.h>
#include <ds/list.h>
#include <fs/fs.h>
#include <string.h>
#include <sys/thread.h>
#include <sync/mutex.h>
#include <mm/kalloc.h>
#include <sys/schedule.h>
#include <arch/x86_64/isr.h>
#include <core/timer.h>
#include <dev/tsc.h>

void handle_signal(signo_t sig, siginfo_t *, void *) {
    debug("%s\n", signal_str[sig - 1]);
}

int setup_signal() {
    sigaction_t act;

    act.sa_flags   = 0;
    act.sa_handler = handle_signal;
    sigsetempty(&act.sa_mask);

    return sigaction(SIGALRM, &act, NULL);
}

void *thread(void *) {
    loop() sched_yield();
}

__noreturn void kthread_main(void) {
    setup_signal();
    thread_builtin_init();

    loop();
}