#include <core/debug.h>
#include <sys/thread.h>

void thread(void) {
} BUILTIN_THREAD(thread, thread, NULL);

__noreturn void kthread_main(void) {
    thread_builtin_init();

    thread_join(2, NULL, NULL);

    debuglog();
    loop() {
    }
}