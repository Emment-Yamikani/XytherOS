#include <core/debug.h>
#include <dev/dev.h>
#include <ds/list.h>
#include <fs/fs.h>
#include <string.h>
#include <sys/thread.h>
#include <sync/mutex.h>

void thread(void) {
    debuglog();
    loop();
} BUILTIN_THREAD(thread, thread, NULL);

void signal_handler(int signo, siginfo_t *siginfo, ucontext_t *uctx) {
    printk("Hello '%s', siginfo: %d, uctx: %p\n", signal_str[signo - 1], siginfo->si_signo, uctx);
}

__noreturn void kthread_main(void) {
    // int err;

    // assert_eq(err = dev_init(), 0, "Failed to start devices, error: %s\n", perror(err));

    // assert_eq(err = vfs_init(), 0, "Failed to initialize VFS!, error: %s\n", perror(err));

    thread_builtin_init(); // start builtin threads.

    // assert_eq(err = proc_spawn_init("/init"), 0, "Error: %s\n", perror(err));

    sigaction_t act;
    act.sa_flags    = SA_SIGINFO;
    act.sa_handler  = signal_handler;
    sigsetempty(&act.sa_mask);

    sigaction(SIGCANCEL, &act, NULL);

    int err;
    assert_eq(err = pthread_kill(2, SIGCANCEL), 0, "Failed to kill thread.\n");
    assert_eq(err = pthread_kill(2, SIGCANCEL), 0, "Failed to kill thread.\n");

    debuglog();
    loop() {
    }
}
