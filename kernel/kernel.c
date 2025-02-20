#include <core/debug.h>
#include <dev/dev.h>
#include <fs/fs.h>
#include <string.h>
#include <sys/thread.h>

void thread(void) {
} BUILTIN_THREAD(thread, thread, NULL);

__noreturn void kthread_main(void) {
    // int err;

    // assert_eq(err = dev_init(), 0, "Failed to start devices, error: %s\n", perror(err));

    // assert_eq(err = vfs_init(), 0, "Failed to initialize VFS!, error: %s\n", perror(err));

    thread_builtin_init(); // start builtin threads.
    
    loop() {
    }
}