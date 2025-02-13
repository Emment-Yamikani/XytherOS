#include <sys/thread.h>
#include <core/debug.h>

void thread(void) {
    loop() debugloc();
}

__noreturn void kthread_main(void) {
    debugloc();
    thread_builtin_init();

    for (int i = 0; i < 200; ++i)
        thread_create(NULL, (void *)thread, NULL, THREAD_CREATE_SCHED, NULL);

    loop() {
    }
}