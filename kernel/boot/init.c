#include <arch/chipset.h>
#include <boot/boot.h>
#include <core/debug.h>
#include <dev/timer.h>
#include <mm/mem.h>
#include <sys/schedule.h>
#include <sys/thread.h>

extern __noreturn void kthread_main(void);

void early_init(void) {
    int err = 0;

    assert_eq(err = vmman.init(), 0,
        "Error[%s]: initializing virtual memory manager.\n", perror(err));

    assert_eq(err = interrupt_controller_init(), 0,
        "Error[%s]: initializing virtual memory manager.\n", perror(err));

    assert_eq(err = timer_init(), 0,
        "Error[%s]: initializing timers.\n", perror(err));

    assert_eq(err = thread_create(NULL, (thread_entry_t)kthread_main, NULL, THREAD_CREATE_SCHED, NULL), 0,
        "Failed to create main kernel thread: err: %s\n", perror(err));

    ap_signal();
    scheduler();

    loop() asm volatile ("pause");
}