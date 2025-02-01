#include <boot/boot.h>
#include <core/assert.h>
#include <core/defs.h>
#include <string.h>

bootinfo_t bootinfo = {0};

void boot_mmap_dump(void) {
    for (u32 i = 0; i < bootinfo.mmapcnt; ++i) {
        printk("mmap(%d): addr: %p, type: %d, size: %10lX(Hex) Bytes\n",
            i,
            bootinfo.mmap[i].addr,
            bootinfo.mmap[i].type,
            bootinfo.mmap[i].size
        );
    }

    printk("memory info:\n"
            "Total:  %d kB\n"
            "Usable: %d KB\n"
            "Used:   %d KB\n",
            bootinfo.total,
            bootinfo.usable,
            bootinfo.total - bootinfo.usable
    );
}

// Swap two boot_mmap_t elements
static void swap_mmap(boot_mmap_t *a, boot_mmap_t *b) {
    boot_mmap_t temp;

    temp.addr = a->addr;
    temp.size = a->size;
    temp.type = a->type;

    a->addr = b->addr;
    a->size = b->size;
    a->type = b->type;

    b->addr = temp.addr;
    b->size = temp.size;
    b->type = temp.type;
}

// Bubble Sort function to sort the memory map array based on addr
static void bubble_sort_mmap(boot_mmap_t *array, size_t count) {
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = 0; j < count - 1 - i; j++) {
            if (array[j].addr > array[j + 1].addr) {
                // Swap elements if they are in the wrong order
                swap_mmap(&array[j], &array[j + 1]);
            }
        }
    }
}

void multiboot_info_process(multiboot_info_t *mbi) {
    boot_mmap_t     *mmap   = bootinfo.mmap;
    
    // Initialize bootinfo with zero
    memset(&bootinfo, 0, sizeof(bootinfo_t));

    // Setup kernel address and size.
    bootinfo.kern_base  = PGROUND(kstart);
    bootinfo.kern_size  = PGROUNDUP(kend - kstart);

    // Get memory information
    bootinfo.memlo      = (usize)mbi->mem_lower;
    bootinfo.memhi      = (usize)mbi->mem_upper;
    bootinfo.total      = M2KiB(1) + mbi->mem_upper;

    // Get memory map information
    if (mbi->flags & MULTIBOOT_INFO_MEM_MAP) {
        mmap_entry_t    *entry  = (mmap_entry_t *)V2HI(mbi->mmap_addr);
        mmap_entry_t    *end    = (mmap_entry_t *)V2HI(mbi->mmap_addr + mbi->mmap_length);

        /** Mark the memory region occupied by the kernel as reversed.*/
        bootinfo.mmapcnt = 1;
        mmap->addr       = bootinfo.kern_base;
        mmap->size       = bootinfo.kern_size;
        mmap->type       = MULTIBOOT_MEMORY_RESERVED;
        mmap += 1;      // move the next mmap slot.

        while (entry < end && bootinfo.mmapcnt < NMMAP) {
            /// Only consider memory within 32bit address space.
            /// And usable memory at and beyond 4GiB.
            if ((entry->addr < GiB(4)) || ((entry->addr >= GiB(4)) && (entry->type == MULTIBOOT_MEMORY_AVAILABLE))) {
                mmap->size  = entry->len;
                mmap->type  = entry->type;
                mmap->addr  = V2HI(entry->addr);

                /* FIXME: 
                Sneaky but it works add to the total the size of
                ACPI-reclaimable memory which according to my observation
                is located at (memhi + 1Mib) which coinsides with first
                memory hole mentioned by the multiboot doumentation.*/
                if (KiB(bootinfo.total) == entry->addr) {
                    bootinfo.total      += B2KiB(mmap->size);
                    bootinfo.hole_addr  = entry->addr;
                    bootinfo.hole_size  = mmap->size;
                } else if ((entry->addr >= GiB(4)) && (entry->type == MULTIBOOT_MEMORY_AVAILABLE)) { // account for usable memory above 4GiB
                    bootinfo.total += B2KiB(mmap->size);
                }

                if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
                    bootinfo.usable += (usize)entry->len;

                bootinfo.mmapcnt++;
                mmap++; // goto the next mmap.
            }
            entry = (mmap_entry_t *)((u64)entry + entry->size + sizeof(entry->size));
        }
    }

    // Subtract to account for space used by kernel.
    bootinfo.usable -= bootinfo.kern_size;
 
    // The first free physical address is the page frame right after the kernel.
    bootinfo.phyaddr = V2LO(PGROUNDUP(kend));

    // Get modules information
    if (mbi->flags & MULTIBOOT_INFO_MODS) {
        mod_entry_t *mod = (mod_entry_t *)V2HI(mbi->mods_addr);
        bootinfo.modcnt  = mbi->mods_count > NMODS ? NMODS : mbi->mods_count;
        for (usize i = 0; i < bootinfo.modcnt; i++) {
            bootinfo.mods[i].addr   = V2HI(mod[i].mod_start);
            bootinfo.mods[i].cmd    = (char *)(V2HI(mod[i].cmdline));
            bootinfo.mods[i].size   = PGROUNDUP(mod[i].mod_end - mod[i].mod_start);

            // Subtract to account for the space used by the module.
            bootinfo.usable         -= bootinfo.mods[i].size;

            // Extend the phyaddr to point to memory after the module.
            bootinfo.phyaddr        = V2LO(bootinfo.mods[i].addr) + bootinfo.mods[i].size;

            // Add module to mmap as reserved
            if (bootinfo.mmapcnt >= NMMAP) {
                panic("Too many mmap entries for modules");
            }

            bootinfo.mmap[bootinfo.mmapcnt].addr = bootinfo.mods[i].addr;
            bootinfo.mmap[bootinfo.mmapcnt].size = bootinfo.mods[i].size;
            bootinfo.mmap[bootinfo.mmapcnt].type = MULTIBOOT_MEMORY_RESERVED;
            bootinfo.mmapcnt++;

        }
    }

    // memory sizes must be in KiB for both usable and total memory.
    bootinfo.usable  = PGROUND(bootinfo.usable) / KiB(1);

    // Sort mmaps' array by sizeof(memory map).
    bubble_sort_mmap(bootinfo.mmap, bootinfo.mmapcnt);

    // Get framebuffer information
    if (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
        bootinfo.fb.bpp    = mbi->framebuffer_bpp;
        bootinfo.fb.type   = mbi->framebuffer_type;
        bootinfo.fb.pitch  = mbi->framebuffer_pitch;
        bootinfo.fb.width  = mbi->framebuffer_width;
        bootinfo.fb.height = mbi->framebuffer_height;
        bootinfo.fb.addr   = V2HI(mbi->framebuffer_addr);
        bootinfo.fb.size   = mbi->framebuffer_height * mbi->framebuffer_pitch;
    }

    // Get bootloader name
    if (mbi->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
        // Store or process the bootloader name if needed
    }
}

/**
 * Allocates physically contiguous memory during early boot.
 *
 * @param size      Minimum allocation size in bytes
 * @param alignment Required alignment boundary (must be power-of-two)
 * @return void*    Virtual address in higher-half memory space
 * @panic           On invalid parameters or memory exhaustion
 */
void *boot_alloc(usize size, usize alignment) {
    void        *addr = NULL;
    uintptr_t   aligned_addr;

    // Validate input size and alignment.
    if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        panic("boot_alloc: Invalid size or alignment.");
        return NULL;
    }

    // Ensure size is rounded up to the nearest alignment size for proper allocation.
    size = (size + alignment - 1) & ~(alignment - 1);

    // Calculate the aligned address.
    aligned_addr = bootinfo.phyaddr;
    if (aligned_addr % alignment != 0) {
        aligned_addr = (aligned_addr + alignment - 1) & ~(alignment - 1);
    }

    // Check if the allocation exceeds the usable memory range or causes overflow.
    if ((aligned_addr + size) < aligned_addr) {
        panic("boot_alloc: Overflow detected.");
        return NULL;
    }

    if ((aligned_addr + size) > KiB(bootinfo.usable)) {
        panic("boot_alloc: (%p) Exceeded usable memory limit: (%d).",
            aligned_addr + size,
            KiB(bootinfo.usable)
        );
        return NULL;
    }

    assert_ne(bootinfo.mmapcnt, NELEM(bootinfo.mmap),
        "OUT OF SPACE TO ADD ANOTHER MMAP\n"
    );

    addr = (void *)V2HI(aligned_addr);

    // Update the bootinfo pointer to reflect the new allocation.
    bootinfo.phyaddr = (uintptr_t)(aligned_addr + size);

    bootinfo.usable -= B2KiB(size); // acount for this used space.

    /** Mark this region as a reserved region. */
    bootinfo.mmap[bootinfo.mmapcnt].size   = size;
    bootinfo.mmap[bootinfo.mmapcnt].addr   = (uintptr_t)addr;
    bootinfo.mmap[bootinfo.mmapcnt++].type = MULTIBOOT_MEMORY_RESERVED;

    return addr;
}
