#include <core/debug.h>
#include <mm/mem.h>
#include <boot/boot.h>

void early_init(void) {
    int err = 0;

    assert_eq(err = vmman.init(), 0,
        "Error[%d]: initializing virtual memory manager.\n", err);

    assert_eq(err = physical_memory_init(), 0,
        "Error[%d]: initializing physical memory manager.\n", err);

    uintptr_t p;
    printk("alloc: %p\n", pmman.alloc());
    printk("alloc: %p\n", p = pmman.alloc());

    printk("alloc: %p\n", pmman.alloc());
    pmman.free(p);

    printk("alloc: %p\n", pmman.alloc());
    loop();
}