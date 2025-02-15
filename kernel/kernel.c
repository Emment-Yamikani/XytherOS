#include <core/debug.h>
#include <sys/thread.h>

void thread(void) {
    debuglog();
}

__noreturn void kthread_main(void) {
    thread_builtin_init();

    for (int i = 0; i < 200; ++i)
        thread_create(NULL, (void *)thread, NULL, THREAD_CREATE_SCHED, NULL);

    loop() {
    }
}