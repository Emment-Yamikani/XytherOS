#include <core/debug.h>
#include <dev/dev.h>
#include <ds/list.h>
#include <fs/fs.h>
#include <string.h>
#include <sys/thread.h>
#include <sync/mutex.h>

__noreturn void kthread_main(void) {
    // int err;

    // assert_eq(err = dev_init(), 0, "Failed to start devices, error: %s\n", perror(err));

    // assert_eq(err = vfs_init(), 0, "Failed to initialize VFS!, error: %s\n", perror(err));

    thread_builtin_init(); // start builtin threads.

    // assert_eq(err = proc_spawn_init("/init"), 0, "Error: %s\n", perror(err));

    signal_t *sig;
    siginfo_t *siginfo = NULL;

    signal_alloc(&sig);

    signal_lock(sig);

    for (int signo = 1; signo < 3; ++signo) {
        siginfo = NULL;
        siginfo_alloc(signo, &siginfo);
        signal_enqueue(sig, siginfo);;
    }

    while (!signal_dequeue(sig, &siginfo)) {
        siginfo_dump(siginfo);
        siginfo_free(siginfo);
        siginfo = NULL;
    }

    signal_free(sig);


    debuglog();
    loop() {
    }
}
