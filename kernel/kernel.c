#include <core/debug.h>
#include <dev/dev.h>
#include <fs/fs.h>
#include <string.h>
#include <sys/thread.h>
#include <sync/mutex.h>
#include <ds/list.h>

#define list_dump(name)                   \
    printk("\nLIST '%s' Dump length[%d]\n", #name, list_length(&name)); \
    list_foreach_entry(car, &name, list)  \
    {                                     \
        printk("data: %d\n", car->data);  \
    }

__noreturn void kthread_main(void) {
    // int err;

    // assert_eq(err = dev_init(), 0, "Failed to start devices, error: %s\n", perror(err));

    // assert_eq(err = vfs_init(), 0, "Failed to initialize VFS!, error: %s\n", perror(err));

    thread_builtin_init(); // start builtin threads.

    loop() {
    }
}
