#include <arch/io.h>
#include <bits/errno.h>
#include <core/debug.h>
#include <dev/bus/pci.h>
#include <ds/hashMap.h>
#include <dev/dev.h>
#include <lib/printk.h>
#include <mm/kalloc.h>
#include <sync/event.h>
#include <sys/thread.h>

static HASHMAP(pci_device_hashMap);
static HASHMAPCTX(pci_device_hashMap_ctx);

static HASHMAP(pci_driver_hashMap);

#define foreach_pci_bus()       for (int bus  = 0; bus  < 256; ++bus)
#define foreach_pci_slot()      for (int slot = 0; slot < 32; ++slot)
#define foreach_pci_function()  for (int func = 0; func < 8;  ++func)

#define foreach_pci_device(item, iter) \
    hashMap_foreach_item(pci_device_hashMap, item, iter) \

#define foreach_pci_driver(item, iter) \
    hashMap_foreach_item(pci_driver_hashMap, item, iter)

#define device_hashMap_lock()           hashMap_lock(pci_device_hashMap);
#define device_hashMap_unlock()         hashMap_unlock(pci_device_hashMap);
#define device_hashMap_trylock()        hashMap_trylock(pci_device_hashMap);
#define device_hashMap_assert_locked()  hashMap_assert_locked(pci_device_hashMap);
#define device_hashMap_recursive_lock() hashMap_recursive_lock(pci_device_hashMap);

#define driver_hashMap_lock()           hashMap_lock(pci_driver_hashMap);
#define driver_hashMap_unlock()         hashMap_unlock(pci_driver_hashMap);
#define driver_hashMap_trylock()        hashMap_trylock(pci_driver_hashMap);
#define driver_hashMap_assert_locked()  hashMap_assert_locked(pci_driver_hashMap);
#define driver_hashMap_recursive_lock() hashMap_recursive_lock(pci_driver_hashMap);


#define VALIDATE_VENDOR(v)      ((v & 0xffff) != 0xffff)
#define INVALID_VENDOR(v)       (!VALIDATE_VENDOR(v))

static hashKey pci_hashMap_hash(hashMap *map, const void *key) {
    hashMap_assert_locked(map);

    pci_devid_t *devid = (pci_devid_t *)key;
    assert(devid, "Invalid key.\n");
    return (devid->device << 16 | devid->vendor);
}

static int pci_hashMap_copy(const void *key, void **ref) {
    if (!key || !ref) {
        return -EINVAL;
    }

    *ref = (void *)key;
    return 0;
}

static void pci_hashMap_destroy(void *key) {
    (void)key;
}

static bool pci_hashMap_compare(const void *key1, const void *key2) {
    assert(key1, "Invalid key1.\n");
    assert(key2, "Invalid key2.\n");
    
    pci_devid_t *devid1 = (pci_devid_t *)key1, *devid2 = (pci_devid_t *)key2;
    return pci_devid_compare(devid1, devid2);
}

/** ----------------------------------------------------- */
/** ------------ Helper functions for hashMap ----------- */
/** ----------------------------------------------------- */

static int pci_hashMap_insert(hashMap *map, void *key, void *data) {
    if (!map || !key || !data) {
        return -EINVAL;
    }

    hashMap_lock(map);
    int err = hashMap_insert(map, key, data);
    hashMap_unlock(map);

    return err;
}

static int pci_hashMap_lookup(hashMap *map, void *key, void **rdata) {
    if (!map || !key) {
        return -EINVAL;
    }

    hashMap_lock(map);
    int err = hashMap_lookup(map, key, rdata);
    hashMap_unlock(map);

    return err;
}

static int pci_hashMap_remove(hashMap *map, void *key) {
    if (!map || !key) {
        return -EINVAL;
    }

    hashMap_lock(map);
    int err = hashMap_remove(map, key);
    hashMap_unlock(map);

    return err;
}

static size_t pci_hashMap_size(hashMap *map) {
    hashMap_lock(map);
    size_t size = hashMap_size(map);
    hashMap_unlock(map);

    return size;
}

static size_t pci_hashMap_capacity(hashMap *map) {
    hashMap_lock(map);
    size_t capacity = hashMap_capacity(map);
    hashMap_unlock(map);

    return capacity;
}

/** ----------------------------------------------------- */
/** ------------ Helper functions for devices ----------- */
/** ----------------------------------------------------- */

int device_hashMap_insert(pci_device_t *device) {
    return pci_hashMap_insert(pci_device_hashMap, &device->devid, device);
}

int device_hashMap_lookup(pci_devid_t *devid, pci_device_t **rdevice) {
    return pci_hashMap_lookup(pci_device_hashMap, devid, (void **)rdevice);
}

int device_hashMap_remove(pci_devid_t *devid) {
    return pci_hashMap_remove(pci_device_hashMap, devid);
}

size_t device_hashMap_size() {
    return pci_hashMap_size(pci_device_hashMap);
}

size_t device_hashMap_capacity() {
    return pci_hashMap_capacity(pci_device_hashMap);
}

int device_into_iter(hashMap *, hashEntry *entry, void *arg) {
    if (!entry || !arg) {
        return -EINVAL;
    }

    device_hashMap_assert_locked();

    pci_driver_t *driver = arg;
    pci_device_t *device = entry->value;

    foreach_take(devid, &driver->devids, driver->devidcnt) {
        if (device->flags & PCI_DEVICE_BOUND) {
            continue;
        }

        if (pci_match_device(device, devid, PCI_MATCH_ALL)) {
            if (driver->bind) {
                int err = driver->bind(device);
                if (err != 0) {
                    return err;
                }
            }

            device->flags |= PCI_DEVICE_BOUND;
        }
    }

    return 0;
}

/** ----------------------------------------------------- */
/** ------------ Helper functions for drivers ----------- */
/** ----------------------------------------------------- */

int driver_hashMap_insert(pci_driver_t *driver) {
    return pci_hashMap_insert(pci_driver_hashMap, &driver->name, driver);
}

int driver_hashMap_lookup(const char *name, pci_driver_t **rdriver) {
    return pci_hashMap_lookup(pci_driver_hashMap, (void *)name, (void **)rdriver);
}

int driver_hashMap_remove(const char *name) {
    return pci_hashMap_remove(pci_driver_hashMap, (void *)name);
}

size_t driver_hashMap_size(void) {
    return pci_hashMap_size(pci_driver_hashMap);
}

size_t driver_hashMap_capacity(void) {
    return pci_hashMap_capacity(pci_driver_hashMap);
}

bool driver_hashMap_empty(void) {
    return driver_hashMap_size() == 0;
}

int driver_into_iter(hashMap *, hashEntry *entry, void *) {
    if (entry == NULL) {
        return -EINVAL;
    }

    driver_hashMap_assert_locked();

    pci_driver_t *driver = entry->value;

    device_hashMap_lock();

    int err = hashMap_into_iter(pci_device_hashMap, device_into_iter, driver);

    device_hashMap_unlock();

    return err;
}

static int match_drivers() {
    driver_hashMap_lock();

    int err = hashMap_into_iter(pci_driver_hashMap, driver_into_iter, NULL);

    driver_hashMap_unlock();
    return err;
}

int pci_scan_bus(void) {
    foreach_pci_bus() {
        foreach_pci_slot() {
            foreach_pci_function() {
                uint32_t retval = pci_scan_device(bus, slot, func, 0);
                if (INVALID_VENDOR(retval)) {
                    continue;
                }

                uint16_t vendorID = retval & 0xFFFF;
                uint16_t deviceID = (retval >> 16) & 0xFFFF;

                pci_device_t *device;
                int err = pci_alloc_device(deviceID, vendorID, bus, slot, func ,&device);
                if (err != 0) {
                    return err;
                }

                if ((err = device_hashMap_insert(device))) {
                    pci_free_device(device);
                    return err;
                }
            }
        }
    }

    return 0;
}

static await_event_t new_driver_event;

int pci_register_driver(pci_driver_t *driver) {
    if (driver == NULL) {
        return -EINVAL;
    }

    int err;
    if ((err = driver_hashMap_insert(driver))) {
        return err;
    }

    await_event_signal(&new_driver_event);

    return 0;
}

static void pci_prober(void) {
    pci_device_hashMap_ctx->hash      = pci_hashMap_hash;
    pci_device_hashMap_ctx->copy      = pci_hashMap_copy;
    pci_device_hashMap_ctx->destroy   = pci_hashMap_destroy;
    pci_device_hashMap_ctx->compare   = pci_hashMap_compare;

    debug("Initializing PCI resources.\n");

    // Initialize PCI device HashMap.
    int err = hashMap_init(pci_device_hashMap, pci_device_hashMap_ctx);
    assert_eq(err, 0, "%s, Failed to initialize pci_device_hashMap.\n", strerror(err));

    // Initialize PCI driver HashMap
    err = hashMap_init(pci_driver_hashMap, NULL);
    assert_eq(err, 0, "%s, Failed to initialize pci_driver_hashMap.\n", strerror(err));

    err = pci_scan_bus(); // Scan the PCI bus.
    assert_eq(err, 0, "%s, An error occured while scanning the PCI bus.\n", strerror(err));

    // Initialize new driver registration event. 
    err = await_event_init(&new_driver_event);
    assert_eq(err, 0, "%s, Failed to initialize the await-event handle.\n", strerror(err));

    debug("Done initializing PCI resources.\n");

    // main loop of PCI prober thread
    loop_and_yield() {
        await_event(&new_driver_event, NULL); // wait for new driver registrations.

        // TODO: make this match ony drivers that haven't been matched yet.
        err = match_drivers(); // try to match all devices

        // If we fail to match any driver to devices panic here.
        assert_eq(err, 0, "%s: Failed to match device-driver pair.\n", strerror(err));
    }
} BUILTIN_THREAD(PCI, pci_prober, NULL);