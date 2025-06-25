#include <arch/chipset.h>
#include <boot/boot.h>
#include <core/debug.h>
#include <dev/timer.h>
#include <fs/fs.h>
#include <mm/mem.h>
#include <sys/schedule.h>
#include <sys/thread.h>

extern __noreturn void kthread_main(void);

void early_init(void) {
    int err = 0;

    printk("\e[2JInitializing kernel...\n");

    assert_eq(err = vmman.init(), 0, "Error[%s]: Initializing virtual memory manager.\n", strerror(err));

    assert_eq(err = interrupt_controller_init(), 0, "Error[%s]: Initializing intterupt controller.\n", strerror(err));

    assert_eq(err = timer_init(), 0, "Error[%s]: Initializing timers.\n", strerror(err));

    assert_eq(err = dev_init(), 0, "Error[%s]: Initializing device subsystem.\n", strerror(err));

    assert_eq(err = vfs_init(), 0, "Error[%s]: Initializing Virtual filesystem.\n", strerror(err));

    ap_signal(); // Inform APs that early initialization is done.

    // TODO: Add everything else to kthread_main().

    err = thread_create(NULL, (thread_entry_t)kthread_main, NULL, THREAD_CREATE_SCHED, NULL);
    assert_eq(err, 0, "Error[%s]: Creating 'main kernel thread'.\n", strerror(err));

    scheduler(); // being executing threads.

    loop() cpu_pause();
}