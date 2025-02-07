#include <sys/thread.h>

__noreturn void kthread_main(void) {
    loop();
}