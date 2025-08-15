#include <core/debug.h>
#include <sys/thread.h>
#include <dev/dev.h>
#include <dev/bus/pci.h>
#include <ds/hashMap.h>
#include <ds/iter.h>
#include <fs/filename.h>
#include <fs/file.h>

int init_kernel_logger(void) {
    mode_t mode = S_IFCHR | 0660;
    int err = vfs_mknod("/dev/tty0", NULL, mode, DEV_T(4, 0));
    assert_eq(err, 0, "%s: Failed to make '/dev/tty0' file node.", strerror(err));
    err = vfs_mknod("/dev/tty1", NULL, mode, DEV_T(4, 1));
    assert_eq(err, 0, "%s: Failed to make '/dev/tty1' file node for kernel logging.", strerror(err));

    return err;
}

__noreturn void kthread_main(void) {
    int err = builtin_thread_init();
    assert_eq(err, 0, "%s: Failed to initialize some or all builtin threads.", strerror(err));

    jiffies_timed_wait(1000);
    printk("\e[2J");

    err = init_kernel_logger();
    assert_eq(err, 0, "%s: Failed to initialize kernel logger.", strerror(err));

    proc_init("/ramfs/init");

    loop_and_yield();
}
