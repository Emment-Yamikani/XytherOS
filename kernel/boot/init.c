#include <core/debug.h>
#include <mm/mem.h>
#include <arch/chipset.h>
#include <boot/boot.h>
#include <sys/schedule.h>
#include <sys/thread.h>

extern __noreturn void kthread_main(void);

void early_init(void) {
    int err = 0;

    assert_eq(err = vmman.init(), 0,
        "Error[%d]: initializing virtual memory manager.\n", err);

    pic_init();
    assert_eq(err = ioapic_init(), 0,
        "Error[%d]: initializing IOAPIC.\n", err
    );

    ap_signal();

    assert_eq(err = thread_create(NULL, (thread_entry_t)kthread_main,
        NULL, THREAD_CREATE_SCHED, NULL), 0,
        "Failed to create main kernel thread: err: %s\n",
        perror(err)
    );

    scheduler();
    loop() asm volatile ("pause");
}