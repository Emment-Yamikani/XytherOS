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
#include <sync/rwlock.h>
#include <dev/cga.h>

void *thread(void *) {
    loop_and_yield();
}

__noreturn void kthread_main(void) {
    thread_builtin_init();

    for (int i = 0; i < 300; ++i) {
        thread_create(NULL, thread, NULL, THREAD_CREATE_SCHED, NULL);
    }

    loop_and_yield() {
    }
}