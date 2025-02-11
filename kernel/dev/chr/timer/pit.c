#include <arch/cpu.h>
#include <arch/traps.h>
#include <arch/chipset.h>
#include <arch/x86_64/asm.h>
#include <core/defs.h>
#include <dev/timer.h>
#include <lib/printk.h>

volatile int intsrc_overide = 0;

#define DATAPORT    0x40
#define CMDPORT     0x43

#define COUNTER(i)  (DATAPORT + i)

#define CH(i)       SHL(i, 6)

#define LOB         1
#define HIB         2
#define LOHI        3
#define HZ          SYS_HZ

#define MODE(x)     SHL(x, 1)
#define RATEGEN     MODE(2)
#define ONESHOT     MODE(1)

void pit_init(void) {
    outb(CMDPORT, LOHI | RATEGEN);
    uint16_t counter = 1193182 / HZ;
    outb(COUNTER(0), (uint8_t)counter);
    inb(0x60);
    outb(COUNTER(0), (uint8_t)(counter >> 8));

    pic_enable(IRQ_PIT0);
    interrupt_controller_enable(IRQ_PIT2, getcpuid());
}

void pit_intr(void) {
    jiffies_update();
}

/* 
 * Wait for a specified number of seconds using the PIT channel 2 in one-shot mode.
 * 
 * @param seconds: Duration to wait (in seconds).
 */
void pit_wait(double seconds) {
    uint8_t port_data;
    uint16_t frequency;
    uint16_t counter;

    // Calculate the frequency in Hz (inverse of the wait time).
    // Ensure that 'seconds' is not zero to avoid division by zero.
    frequency = (uint16_t)(1.0 / seconds);

    // Compute the counter value for the PIT given the base frequency (1,193,182 Hz).
    counter = 1193182 / frequency;

    // Configure port 0x61: Clear bit 1 (mask with 0xFD) and set bit 0.
    port_data = (inb(0x61) & 0xFD) | 0x01;
    outb(0x61, port_data);

    // Set up PIT channel 2:
    //   - CH(2): selects channel 2.
    //   - LOHI: indicates that the counter value will be sent as two bytes (low then high).
    //   - ONESHOT: configures the channel in one-shot mode.
    outb(CMDPORT, CH(2) | LOHI | ONESHOT);

    // Send the counter value in two parts: low byte first, then high byte.
    outb(COUNTER(2), (uint8_t)counter);
    // A dummy read from port 0x60 (often used to create a short delay or acknowledge an interrupt).
    (void)inb(0x60);
    outb(COUNTER(2), (uint8_t)(counter >> 8));

    // Toggle the control bits on port 0x61:
    // Clear bit 0, then set it again.
    port_data = inb(0x61) & 0xFE;
    outb(0x61, port_data);
    outb(0x61, port_data | 0x01);

    // Wait until the PIT channel 2 indicates that the time interval has elapsed.
    // The loop polls port 0x61 until bit 5 (0x20) is set.
    while (!(inb(0x61) & 0x20))
        ;
}
