#pragma once

#include <core/types.h>
#include <sync/atomic.h>

typedef enum {
    IOADDR_PORT8    = 1,
    IOADDR_PORT16   = 2,
    IOADDR_PORT32   = 3,

    IOADDR_MMIO8    = 4,
    IOADDR_MMIO16   = 5,
    IOADDR_MMIO32   = 6,
    IOADDR_MMIO64   = 7,
} ioaddr_type_t;

typedef struct ioaddr {
    ioaddr_type_t   type;
    uintptr_t       addr;
} ioaddr_t;

extern const char *ioaddr_type(ioaddr_t *ioaddr);

extern int io_read8(ioaddr_t *ioaddr, off_t off, uint8_t *out);
extern int io_read16(ioaddr_t *ioaddr, off_t off, uint16_t *out);
extern int io_read32(ioaddr_t *ioaddr, off_t off, uint32_t *out);
extern int io_read64(ioaddr_t *ioaddr, off_t off, uint64_t *out);

extern int io_write8(ioaddr_t *ioaddr, off_t off, uint8_t val);
extern int io_write16(ioaddr_t *ioaddr, off_t off, uint16_t val);
extern int io_write32(ioaddr_t *ioaddr, off_t off, uint32_t val);
extern int io_write64(ioaddr_t *ioaddr, off_t off, uint64_t val);
