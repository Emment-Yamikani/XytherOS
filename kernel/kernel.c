#include <core/debug.h>
#include <dev/dev.h>
#include <ds/list.h>
#include <fs/fs.h>
#include <string.h>
#include <sys/thread.h>
#include <sync/mutex.h>
#include <mm/kalloc.h>
#include <sys/schedule.h>
#include <arch/x86_64/isr.h>
#include <core/timer.h>
#include <ds/kfifo.h>

__noreturn void kthread_main(void) {
    
    loop();
}