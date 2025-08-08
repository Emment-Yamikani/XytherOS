#pragma once

#include <core/defs.h>
#include <core/types.h>

extern uint32_t pci_read(uintptr_t addr);
extern void     pci_write(uint32_t addr, uintptr_t value);
extern uint32_t pci_scan_device(uint8_t bus, uint8_t slot, uint8_t fun, uint8_t off);

typedef struct {
    uint16_t    device;
    uint16_t    vendor;
    uint8_t     baseclass;
    uint8_t     subclass;
    uint8_t     progIF;
    uint8_t     revision;
} pci_devid_t;

extern bool pci_devid_compare(pci_devid_t *devid0, pci_devid_t *devid1);

#define PCI_DEVICE_BOUND 0x1

typedef struct {
    uint8_t      bus;
    uint8_t      slot;
    uint8_t      func;
    unsigned int flags;
    pci_devid_t  devid;
} pci_device_t;

extern void pci_free_device(pci_device_t *device);
extern int pci_alloc_device(uint16_t deviceID, uint16_t vendorID, uint8_t bus, uint8_t slot, uint8_t func, pci_device_t **ret);

typedef struct {
    char        name[64];
    size_t      devidcnt;
    pci_devid_t *devids;
    int         (*bind)(pci_device_t *dev);
} pci_driver_t;

extern int pci_register_driver(pci_driver_t *driver);

#define PCI_MATCH_ALL      0  // All fields must match
#define PCI_MATCH_ANY      1  // Any matching field is sufficient
#define PCI_MATCH_CLASS    2  // Only class/subclass must match
#define PCI_MATCH_DEVICEID 3  // Only vendor and device ID must match

extern bool pci_match_device(pci_device_t *device, pci_devid_t *devid, int how);