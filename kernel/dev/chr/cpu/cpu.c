#include <dev/dev.h>


static int cpudev_init(void) {
    return 0;
}

BUILTIN_DEVICE(cpu, cpudev_init, NULL, NULL);