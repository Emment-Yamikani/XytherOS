#include <dev/dev.h>
#include <sys/thread.h>

extern int device_mux_init(void);

int dev_init(void) {
    int err = device_mux_init();

    if (err != 0) {
        return err;
    }
    
    foreach_builtin_device() {
        if (dev->init && (err = (dev->init)(dev->arg))) {
            return err;
        }
    }

    return 0;
} BUILTIN_THREAD(device_subsys, dev_init, NULL);