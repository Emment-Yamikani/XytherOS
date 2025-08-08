#include <arch/io.h>
#include <bits/errno.h>
#include <dev/bus/pci.h>
#include <dev/dev.h>
#include <lib/printk.h>
#include <mm/kalloc.h>
#include <sync/event.h>
#include <sys/thread.h>

static ioaddr_t pci_cmd = {.addr = 0xCF8, .type = IOADDR_PORT32};
static ioaddr_t pci_data= {.addr = 0xCFC, .type = IOADDR_PORT32};

uint32_t pci_read(uintptr_t address) {
    uint32_t data = 0;
    io_write32(&pci_cmd, 0, address);
    io_read32(&pci_data, 0, &data);
    return data;
}

void pci_write(uint32_t address, uintptr_t value) {
    io_write32(&pci_cmd, 0, address);
    io_write32(&pci_data, 0, value);
}

uint32_t pci_scan_device(uint8_t bus, uint8_t slot, uint8_t function, uint8_t off) {
    u32 address = (1 << 31) | (bus << 16 ) | (slot << 11) | (function << 8) | (off & 0xfc);
    return pci_read(address);
}

int pci_alloc_device(uint16_t deviceID, uint16_t vendorID, uint8_t bus, uint8_t slot, uint8_t func, pci_device_t **ret) {
    if (ret == NULL || vendorID == 0xFFFF) {
        return -EINVAL;
    }

    pci_device_t *device = kzalloc(sizeof *device);
    if (device == NULL) {
        return -ENOMEM;
    }

    uint32_t retval = pci_scan_device(bus, slot, func, 8);

    uint8_t revision  = (retval >> 0x00) & 0xff;
    uint8_t progIF    = (retval >> 0x08) & 0xff;
    uint8_t subclass  = (retval >> 0x10) & 0xff;
    uint8_t baseclass = (retval >> 0x18) & 0xff;

    device->devid   = (pci_devid_t) {
        .vendor     = vendorID,
        .device     = deviceID,
        .baseclass  = baseclass,
        .subclass   = subclass,
        .progIF     = progIF,
        .revision   = revision
    };

    device->bus     = bus;
    device->slot    = slot;
    device->func    = func;

    *ret = device;
    return 0;
}

void pci_free_device(pci_device_t *device) {
    if (device == NULL) {
        return;
    }

    kfree(device);
}

bool pci_devid_compare(pci_devid_t *devid0, pci_devid_t *devid1) {
    assert(devid0, "Invalid devid0.\n");
    assert(devid1, "Invalid devid1.\n");

    return  devid0->device    == devid1->device   &&
            devid0->vendor    == devid1->vendor   &&
            devid0->baseclass == devid1->baseclass&&
            devid0->subclass  == devid1->subclass;
}

bool pci_match_device(pci_device_t *device, pci_devid_t *devid, int how) {
    assert(devid,  "Invalid devid.\n");
    assert(device, "Invalid device.\n");

    switch (how) {
        case PCI_MATCH_ALL:
            return pci_devid_compare(&device->devid, devid);

        case PCI_MATCH_DEVICEID:
            return device->devid.vendor   == devid->vendor &&
                   device->devid.device   == devid->device;

        case PCI_MATCH_CLASS:
            return device->devid.baseclass== devid->baseclass &&
                   device->devid.subclass == devid->subclass;

        case PCI_MATCH_ANY:
            return (device->devid.vendor  == devid->vendor) ||
                   (device->devid.device  == devid->device) ||
                   (device->devid.baseclass== devid->baseclass) ||
                   (device->devid.subclass== devid->subclass);

        default:   return false;
    }
}