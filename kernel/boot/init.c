#include <core/assert.h>
#include <mm/mem.h>

void early_init(void) {
    int err = 0;

    assert_eq(err = physical_memory_init(), 0, "Error initializing physical memory manager.\n");


    loop();
}