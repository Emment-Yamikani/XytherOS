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

static void callback(signo_t sig) {
    printk("%s\n", signal_str[sig - 1]);
}

__noreturn void kthread_main(void) {
    BUILTIN_THREAD_ANNOUNCE("");

    tmrid_t id;
    tmr_desc_t tmr;

    sigaction_t act;

    act.sa_flags   = 0;
    sigsetempty(&act.sa_mask);
    act.sa_handler = (void *)callback;

    sigaction(SIGALRM, &act, NULL);

    tmr.td_expiry.tv_sec    = 5;
    tmr.td_expiry.tv_nsec   = 0;
    tmr.td_inteval.tv_sec   = 0;
    tmr.td_func             = NULL;
    tmr.td_signo            = SIGALRM;
    tmr.td_inteval.tv_nsec  = 1000000000;

    timer_create(&tmr, &id);

    timespec_t ts;
    ts.tv_nsec = 0;
    ts.tv_sec  = 10;

    printk("nanosleep()\n");
    int err = nanosleep(&ts, NULL);
    printk("err: %s\n", perror(err));

    debuglog();
    loop();
}
