#include <sys/thread.h>
#include <core/debug.h>

void builtin(void) {
    debugloc();
    loop();
} BUILTIN_THREAD(builtin, builtin, NULL);

__noreturn void kthread_main(void) {
    thread_builtin_init();

    loop() {

    }
}