#include <core/debug.h>
#include <sys/thread.h>
#include <dev/dev.h>

__noreturn void kthread_main(void) {
    thread_builtin_init();

    loop_and_yield();
}