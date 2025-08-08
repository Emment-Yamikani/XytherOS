#include <arch/io.h>
#include <bits/errno.h>

static inline uint8_t __inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %%dx, %%al" : "=a"(ret) : "d"(port));
    return ret;
}

static inline uint16_t __inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %%dx, %%ax" : "=a"(ret) : "dN"(port));
    return ret;
}

static inline uint32_t __inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %%dx, %%eax" : "=a"(ret) : "d"(port));
    return ret;
}

static inline void __outb(uint16_t port, uint8_t value) {
    asm volatile("outb %%al, %%dx" ::"d"((port)), "a"((value)));
}

static inline void __outw(uint16_t port, uint16_t value) {
    asm volatile("outw %%ax, %%dx" ::"d"(port), "a"(value));
}

static inline void __outl(uint16_t port, uint32_t value) {
    asm volatile("outl %%eax, %%dx" ::"d"(port), "a"(value));
}

static inline void __io_wait() {
    __outb(0x80, 0);
}

#define __mmio_read8(addr)  (*(volatile uint8_t  *)(addr))
#define __mmio_read16(addr) (*(volatile uint16_t *)(addr))
#define __mmio_read32(addr) (*(volatile uint32_t *)(addr))
#define __mmio_read64(addr) (*(volatile uint64_t *)(addr))

#define __mmio_write8(addr, val)  (*(volatile uint8_t  *)(addr) = (val))
#define __mmio_write16(addr, val) (*(volatile uint16_t *)(addr) = (val))
#define __mmio_write32(addr, val) (*(volatile uint32_t *)(addr) = (val))
#define __mmio_write64(addr, val) (*(volatile uint64_t *)(addr) = (val))

const char *ioaddr_type(ioaddr_t *ioaddr) {
    switch (ioaddr->type) {
        case IOADDR_PORT8:
            return "pio8";
        case IOADDR_PORT16:
            return "pio16";
        case IOADDR_PORT32:
            return "pio32";

        case IOADDR_MMIO8:
            return "mmio8";
        case IOADDR_MMIO16:
            return "mmio16";
        case IOADDR_MMIO32:
            return "mmio32";
        case IOADDR_MMIO64:
            return "mmio64";
        default:
            return "undefined ioaddr";
    }
}

static inline int io_read(ioaddr_t *ioaddr, off_t off, volatile void *out) {
    if (ioaddr == NULL || ioaddr->addr == 0 || out == NULL) return -EINVAL;

    switch ((ioaddr)->type) {
        case IOADDR_PORT8:
            *((volatile uint8_t *)out) = __inb((ioaddr)->addr + (off));
            return 0;

        case IOADDR_PORT16:
            *((volatile uint16_t *)out) = __inw((ioaddr)->addr + (off));
            return 0;

        case IOADDR_PORT32:
            *((volatile uint32_t *)out) = __inl((ioaddr)->addr + (off));
            return 0;


        case IOADDR_MMIO8:
            *((volatile uint8_t *)out) = __mmio_read8((ioaddr)->addr + (off));
            return 0;

        case IOADDR_MMIO16:
            *((volatile uint16_t *)out) = __mmio_read16((ioaddr)->addr + ((off)));
            return 0;

        case IOADDR_MMIO32:
            *((volatile uint32_t *)out) = __mmio_read32((ioaddr)->addr + ((off)));
            return 0;

        case IOADDR_MMIO64:
            *((volatile uint64_t *)out) = __mmio_read64((ioaddr)->addr + ((off)));
            return 0;

        default: return -EINVAL;
    }
}

int io_read8(ioaddr_t *ioaddr, off_t off, uint8_t *out) {
    return io_read(ioaddr, off, (volatile void *)out);
}

int io_read16(ioaddr_t *ioaddr, off_t off, uint16_t *out) {
    return io_read(ioaddr, off, (volatile void *)out);
}

int io_read32(ioaddr_t *ioaddr, off_t off, uint32_t *out) {
    return io_read(ioaddr, off, (volatile void *)out);
}

int io_read64(ioaddr_t *ioaddr, off_t off, uint64_t *out) {
    return io_read(ioaddr, off, (volatile void *)out);
}

static inline int io_write(ioaddr_t *ioaddr, off_t off, uintptr_t val) {
    if (ioaddr == NULL || ioaddr->addr == 0) return -EINVAL;

    switch ((ioaddr)->type) {
        case IOADDR_PORT8:
            __outb((ioaddr)->addr + (off), (uint8_t)(val));
            return 0;

        case IOADDR_PORT16:
            __outw((ioaddr)->addr + (off), (uint16_t)(val));
            return 0;

        case IOADDR_PORT32:
            __outl((ioaddr)->addr + (off), (uint32_t)(val));
            return 0;


        case IOADDR_MMIO8:
            __mmio_write8((ioaddr)->addr + (off), (val));
            return 0;

        case IOADDR_MMIO16:
            __mmio_write16((ioaddr)->addr + ((off)), (val));
            return 0;

        case IOADDR_MMIO32:
            __mmio_write32((ioaddr)->addr + ((off)), (val));
            return 0;

        case IOADDR_MMIO64:
            __mmio_write64((ioaddr)->addr + ((off)), (val));
            return 0;
        
        default: return -EINVAL;
    }
}

int io_write8(ioaddr_t *ioaddr, off_t off, uint8_t val) {
    return io_write(ioaddr, off, val);
}

int io_write16(ioaddr_t *ioaddr, off_t off, uint16_t val) {
    return io_write(ioaddr, off, val);
}

int io_write32(ioaddr_t *ioaddr, off_t off, uint32_t val) {
    return io_write(ioaddr, off, val);
}

int io_write64(ioaddr_t *ioaddr, off_t off, uint64_t val) {
    return io_write(ioaddr, off, val);
}