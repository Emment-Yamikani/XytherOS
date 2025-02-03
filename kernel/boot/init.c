#include <core/debug.h>
#include <mm/mem.h>
#include <arch/chipset.h>
#include <boot/boot.h>

void early_init(void) {
    int err = 0;

    assert_eq(err = vmman.init(), 0,
        "Error[%d]: initializing virtual memory manager.\n", err);

    pic_init();
    assert_eq(err = ioapic_init(), 0,
        "Error[%d]: initializing IOAPIC.\n", err
    );

    ap_signal();
    loop() asm volatile ("pause");
}