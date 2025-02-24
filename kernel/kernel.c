#include <core/debug.h>
#include <dev/dev.h>
#include <fs/fs.h>
#include <fs/dent.h>
#include <string.h>
#include <sys/thread.h>
#include <sync/mutex.h>
#include <ds/list.h>


__noreturn void kthread_main(void) {
    // int err;

    // assert_eq(err = dev_init(), 0, "Failed to start devices, error: %s\n", perror(err));

    // assert_eq(err = vfs_init(), 0, "Failed to initialize VFS!, error: %s\n", perror(err));

    thread_builtin_init(); // start builtin threads.

    Dentry root, *child;

    dentry_init("root", &root);

    dentry_alloc("bin", &child);
    dentry_bind(&root, child);
    dentry_close(child);

    dentry_alloc("etc", &child);
    dentry_bind(&root, child);
    dentry_close(child);

    dentry_alloc("src", &child);
    dentry_bind(&root, child);
    dentry_close(child);

    dentry_alloc("tmp", &child);
    dentry_bind(&root, child);
    dentry_close(child);
    
    dentry_lookup(&root, "bin", &child);

    printk("%s/%s/ [count: %d]\n", child->d_parent->d_name, child->d_name, child->d_count);

    loop() {
    }
}
