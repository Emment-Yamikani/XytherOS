#include <core/debug.h>
#include <dev/dev.h>
#include <fs/fs.h>
#include <string.h>
#include <sys/thread.h>
#include <sync/mutex.h>

void thread(void) {
    loop() if (current_iscanceled()) thread_exit(EINTR);
} BUILTIN_THREAD(thread, thread, NULL);

void test_lock(mtx_t *m) {
    mtx_lock(m);

    debuglog();
    mtx_unlock(m);
    debuglog();
}

    MTX(lock);
    MTX(lockx);
__noreturn void kthread_main(void) {
    // int err;

    // assert_eq(err = dev_init(), 0, "Failed to start devices, error: %s\n", perror(err));

    // assert_eq(err = vfs_init(), 0, "Failed to initialize VFS!, error: %s\n", perror(err));

    thread_builtin_init(); // start builtin threads.


    printk("lock: %p, lockx: %p\n", lock, lockx);

    mtx_lock(lock);
    thread_create(NULL, (void *)test_lock, lock, THREAD_CREATE_SCHED, NULL);

    thread_cancel(2);
    thread_info_t info;
    thread_join(2, &info, NULL);

    thread_info_dump(&info);

    mtx_recursive_lock(lock);

    mtx_unlock(lock);
    mtx_unlock(lock);

    loop() {
    }
}