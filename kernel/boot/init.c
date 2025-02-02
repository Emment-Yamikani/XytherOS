#include <core/debug.h>
#include <mm/mem.h>
#include <boot/boot.h>

void early_init(void) {
    int err = 0;

    assert_eq(err = physical_memory_init(), 0,
        "Error initializing physical memory manager. error: %d\n", err);

    loop() {
        uintptr_t p;
        assert(p = pmman.alloc(), "ENOMEM\n");
        printk("alloc: %p\n", p);
    }
    loop();
}