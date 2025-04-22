#pragma once


#include <arch/cpu.h>
#include <boot/multiboot.h>
#include <core/types.h>
#include <dev/fb.h>

#define NMODS   64
#define NMMAP   64 + NCPU

typedef struct {
    uintptr_t   addr;   // address at which module is loaded.
    usize       size;   // size of the module.
    char        *cmd;   // command line passed with the module
} mod_t;

typedef struct {
    uintptr_t   addr;   // address at which the memory map starts.
    usize       size;   // size of the memory map.
    int         type;   // type of memory this mmap describes.
} boot_mmap_t;

#define BOOT_ALLOC_WATERMARK    0x8932CD33DEADFAAFul

typedef struct {
    ulong       watermark;  // guardrail for the bump allocator.
    usize       total;      // total available memory.
    usize       usable;     // Size of usable physical memory.
    usize       memlo;      // Size of lower memory.
    usize       memhi;      // Size of Higher memory.
    uintptr_t   hole_addr;  // Addr of the first Memory Hole.
    usize       hole_size;  // Size of the first Memory Hole.

    struct {
        u8          type;
        uintptr_t   addr;
        u32         pitch;
        u32         width;
        u32         height;
        usize       size;
        u32         bpp;

        struct fb_bitfield red;
        struct fb_bitfield blue;
        struct fb_bitfield green;
        struct fb_bitfield resv;
    } fb /* framebuffer data returned by bootloader*/;

    uintptr_t       phyaddr;    // first free physical address. 
    boot_mmap_t     mmap[NMMAP];// array of memory maps.
    usize           mmapcnt;    // # of memory maps
    
    mod_t           mods[NMODS];// array of available modules.
    usize           modcnt;     // # of modules
    
    uintptr_t       kern_base;
    usize           kern_size;
} bootinfo_t;

typedef multiboot_module_t mod_entry_t;
typedef multiboot_memory_map_t mmap_entry_t;

extern bootinfo_t bootinfo;

#define KERNEL_BASE             (bootinfo.kern_base)
#define KERNEL_SIZE             (bootinfo.kern_size)
#define KERNEL_END              (KERNEL_BASE + KERNEL_SIZE)

#define is_kernel_addr(addr)    ((addr >= KERNEL_BASE) && (addr < KERNEL_END))

extern void boot_mmap_dump(void);

/**
 * Allocates physically contiguous memory during early boot.
 *
 * @param size      Minimum allocation size in bytes
 * @param alignment Required alignment boundary (must be power-of-two)
 * @return void*    Virtual address in higher-half memory space
 * @panic           On invalid parameters or memory exhaustion
 */
void *boot_alloc(usize size, usize alignment);