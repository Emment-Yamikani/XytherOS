#include <core/debug.h>
#include <sys/thread.h>
#include <dev/dev.h>
#include <dev/bus/pci.h>
#include <ds/hashMap.h>
#include <ds/iter.h>

hashKey hash(hashMap *, const void *key) {
    return (hashKey)key;
}

bool compare(const void *key0, const void *key1) {
    return key0 == key1;
}

void *copy(const void *key) {
    return (void *)key;
}

void destroy(void *) {

}

hashMapCtx ctx = {
    .copy    = copy,
    .hash    = hash,
    .compare = compare,
    .destroy = destroy,
};

__noreturn void kthread_main(void) {
    thread_builtin_init();
    jiffies_sleep(100, NULL);

    

    loop_and_yield();
}