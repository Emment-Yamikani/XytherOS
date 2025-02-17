#include <core/debug.h>
#include <sys/thread.h>

void thread(void) {
    loop() debuglog();
}

__noreturn void kthread_main(void) {
    thread_builtin_init();

    for (int i = 0; i < 500; ++i)
        thread_create(NULL, (void *)thread, NULL, THREAD_CREATE_SCHED, NULL);

    loop() {
    }
}