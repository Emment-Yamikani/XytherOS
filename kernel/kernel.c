#include <sys/thread.h>

__noreturn void kthread_main(void) {
    int i = 0;
    loop() printk("Hello, World: %d\n", i++);
}