#include <arch/paging.h>
#include <bits/errno.h>
#include <boot/boot.h>
#include <core/debug.h>
#include <mm/zone.h>
#include <string.h>
#include <sys/thread.h>

zone_t zones[NZONE];

const char *str_zone[] = {
    "DMA", "NORM", "HOLE", "HIGH", NULL,
};

void zone_dump(zone_t *zone) {
    assert(zone, "zerror: No physical memory zone specified\n");
    printk("\nZONE: %s\n"
            "Array:  %16p\n"
            "Size:   %16d KiB\n"
            "Free:   %16d pages\n"
            "Used:   %16d pages\n"
            "Total:  %16d pages\n"
            "Start:  %16p\n"
            "End:    %16p\n",
            str_zone[zone - zones],
            zone->pages,
            zone->size / KiB(1),
            zone->npages - zone->upages,
            zone->upages,
            zone->npages,
            zone->start,
            zone->start + zone->size
    );
}


int getzone_byaddr(uintptr_t paddr, usize size, zone_t **ppz) {
    if (ppz == NULL) {
        return -EINVAL;
    }
    
    for (zone_t *zone = zones; zone < &zones[NZONE]; ++zone) {
        zone_lock(zone);
        if ((paddr >= zone->start) && zone_isvalid(zone) &&
                ((paddr + size) <= (zone->start + zone->size))) {
            *ppz = zone;
            return 0;
        }
        zone_unlock(zone);
    }

    return -ENOENT;
}

int getzone_bypage(page_t *page, zone_t **ppz) {
    if (page == NULL || ppz == NULL) {
        return -EINVAL;
    }
    
    for (zone_t *zone = zones; zone < &zones[NZONE]; ++zone) {
        zone_lock(zone);
        if ((page >= zone->pages) && zone_isvalid(zone) &&
                (page < &zone->pages[zone->npages])) {
            *ppz = zone;
            return 0;
        }
        zone_unlock(zone);
    }

    return -ENOENT;
}

int getzone_byindex(int zone_i, zone_t **ref) {
    zone_t *zone = NULL;

    if ((zone_i < 0) || (zone_i > (int)NELEM(zones))) {
        return -EINVAL;
    }

    zone = &zones[zone_i];
    zone_lock(zone);
    // printk("requesting zone(%d): %p ,flags: %d\n", zone_i, zone, zone->flags);
    if (!zone_isvalid(zone)) {
        zone_unlock(zone);
        return -EINVAL;
    }

    *ref = zone;
    return 0;
}

static inline int zone_alloc_buffers(zone_t *zone) {
    if (!zone_size(zone)) {
        return 0;
    }

    usize bitmap_size = ((zone->npages + BITS_PER_USIZE - 1) / BITS_PER_USIZE) * sizeof (usize);
    bitmap_size = PGROUNDUP(bitmap_size);
    const u64 *bitmap = boot_alloc(bitmap_size, PGSZ);
    if (!bitmap) return -ENOMEM;

    int err = bitmap_init(&zone->bitmap, (usize *)bitmap, zone->npages);
    if (err) return err;

    size_t size = PGROUNDUP(zone->npages * sizeof(page_t));
    const page_t *pages = (page_t *)boot_alloc(size, PGSZ);
    if (!pages) return -ENOMEM;

    page_watermark_t watermark;
    switch (getzone_index(zone)) {
        case ZONEi_DMA:  watermark = PGWM_DMA_POOL; break;
        case ZONEi_NORM: watermark = PGWM_NORM_POOL; break;
        case ZONEi_HOLE: watermark = PGWM_HOLE_POOL; break;
        case ZONEi_HIGH: watermark = PGWM_HIGH_POOL; break;
    }

    zone->pages = (page_t *)pages;
    for (usize idx = 0; idx < zone->npages; ++idx) {
        zone->pages[idx] = (page_t) {
            .flags      = 0,
            .mapcnt     = 0,
            .refcnt     = 0,
            .icache     = NULL,
            .watermark  = watermark
        };
    }

    // mark zone as valid for use.
    zone_flags_set(zone, ZONE_VALID);
    return 0;
}

static inline int zone_set_address_range(zone_t *zone, uintptr_t addr, usize size) {
    // enure that the previous zone is initialize before this one.
    if ((getzone_index(zone) > 0) && !((zone - 1)->flags & ZONE_VALID)) {
        return -EINVAL;
    }

    zone->start  = addr;
    zone->size   = size;
    zone->npages = NPAGE(size);
    
    int err = zone_alloc_buffers(zone);
    if (err) {
        zone->start = 0;
        zone->size = 0;
        zone->npages = 0;
    }

    return err;
}

/**
 * Initialize a memory zone based on the available memory.
 *
 * @param zone Pointer to the zone to initialize.
 * @param memsz Pointer to the remaining memory size in KiB.
 * @return 0 on success, -EINVAL if the previous zone isn't valid.
 */
static int zone_enumerate(zone_t *zone, usize *memsz) {
    if (zone == NULL) {
        return -EINVAL;
    }

    zone_assert_locked(zone);

    // debug("Initializing zone %s: remaining memory size %lu KiB.\n", str_zone[zone - zones], *memsz);
    usize size = 0;
    uintptr_t addr = 0;

    switch (zone - zones) {
    case ZONEi_DMA:
        addr = 0; // DMA zone starts at 0x0
        size = (*memsz > M2KiB(16)) ? MiB(16) : KiB(*memsz);
        break;
    case ZONEi_NORM:
        /// manipulate the available memory size.
        /// if available memory size is large split it.
        /// else use as is.
        /// memsz is in KiB.
        size = (*memsz > M2KiB(2032)) ? MiB(2032) : KiB(*memsz);

        /// set this zone starts @ 16MiB.
        addr = MiB(16);

        break;
    case ZONEi_HOLE:
        /** We add 1 MiB because multiboot says;
         * "The value returned for upper memory is
         * maximally the address of the first
         * upper memory hole minus 1 megabyte.
         * It is not guaranteed to be this value".*/
        size = (KiB((bootinfo.memhi + M2KiB(1))) + bootinfo.hole_size) - GiB(2);

        /// set this zone starts @ 2GiB.
        addr = GiB(2);

        break;
    case ZONEi_HIGH:
        /// for HIGH memory zone, no spliting is needed.
        size  = KiB(*memsz);

        /// set this zone starts @ 4GiB.
        addr = GiB(4);

        break;
    default: return -EINVAL;
    }

    int err = zone_set_address_range(zone, addr, size);
    if (err) return err;

    *memsz -= B2KiB(zone->size);

    // debug("Initialized zone %s[%p: %X]: remaining memory size %lu KiB.\n",
    //       str_zone[zone - zones], zone->start, zone->size, *memsz);

    return 0;
}

static void initialize_zone_struct(zone_t *zone) {
    memset(zone, 0, sizeof(*zone));

    zone->queue     = QUEUE_INIT();
    zone->bitmap    = BITMAP_INIT();
    zone->lock      = SPINLOCK_INIT();
}

// Helper function to initialize individual zones.
static int initialize_memory_zone(zone_t *zone, usize *memsz) {
    int err;

    initialize_zone_struct(zone);
    zone_lock(zone);

    if (*memsz != 0) {
        if ((err = zone_enumerate(zone, memsz))) {
            printk("Failed to init zone. err: %d\n", err);
            zone_unlock(zone);
            return err;
        }
    }

    zone_unlock(zone);
    return 0;
}

// Helper function to process pages within a memory range
static int mark_reserved_pages(zone_t *zone, uintptr_t addr, usize size) {
    usize   np      = 0;
    page_t  *page   = NULL;

    if (zone == NULL) {
        return -EINVAL;
    }

    page = zone->pages + NPAGE(addr - zone->start);
    for (np = NPAGE(size); np; --np, page++, addr += PGSZ) {
        bitmap_set(&zone->bitmap, page - zone->pages, 1);

        if (page->refcnt == 0) {
            zone->upages += 1; // Increment number of used pages.
        } else {
            printk("%s(): %s:%d: [NOTE]: page(%p)->refcnt == %d?\n",
                   __func__, __FILE__, __LINE__, addr, page->refcnt);
        }

        page->refcnt += 1;
        page->mapcnt += 1;
        page->watermark = PGWM_RESERVED;
        page_setflags(page, PG_R | PG_X); // Read and exec only.
    }

    return 0;
}

// Helper function to process a memory map region.
static int process_memory_map(boot_mmap_t *map) {
    int         err     = 0;
    uintptr_t   addr    = 0;
    usize       size    = 0;
    zone_t      *zone   = NULL;

    // Skip available regions or regions outside the physical memory.
    if ((map->addr >= V2HI(MEMMIO)) || (map->type == MULTIBOOT_MEMORY_AVAILABLE)) {
        return 0;
    }

    size = map->size;
    addr = PGROUND(V2LO(map->addr));

    if ((err = getzone_byaddr(addr, size, &zone))) {
        panic("PANIC: %s(): %s:%d: Couldn't get zone for mmap[%p: %d], err: %d\n",
            __func__, __FILE__, __LINE__, addr, size, err);
    }

    if ((err = mark_reserved_pages(zone, addr, size))) {
        panic("PANIC: %s(): %s:%d: Couldn't process memory region[%p: %ld]. err: %d\n",
            __func__, __FILE__, __LINE__, addr, size, err);
    }

    zone_unlock(zone);
    return 0;
}

// Main zones_init function.
int zones_init(void) {
    int         err     = 0;
    boot_mmap_t *map    = NULL;
    zone_t      *zone   = NULL;
    usize       memsz   = bootinfo.total;

    // printk("Initializing memory zones...\n");

    // Initialize zones.
    for (zone = zones; zone < &zones[NZONE]; ++zone) {
        if ((err = initialize_memory_zone(zone, &memsz))) {
            return err;
        }
    }

    // Process memory map regions.
    for (map = bootinfo.mmap; map < &bootinfo.mmap[bootinfo.mmapcnt]; ++map) {
        if ((err = process_memory_map(map))) {
            return err;
        }
    }

    // Set the bump allocator guardrail.
    atomic_set(&bootinfo.watermark, BOOT_ALLOC_WATERMARK);
    // printk("Memory zones initialized.\n");
    return 0;
}

static int zones_protect_sections(void) {
    static atomic_ulong inits = -1;

    assert_eq(atomic_read(&inits), (ulong)(-1), "Multiple inits detected\n");
    atomic_set(&inits, 0);

    const uintptr_t section_ranges[6][2] = {
        {(uintptr_t)__trampoline_section, (uintptr_t)__trampoline_section_end},
        {(uintptr_t)__kernel_start, (uintptr_t)__kernel_readonly_end},
        {(uintptr_t)__text_section, (uintptr_t)__text_section_end},
        {(uintptr_t)__rodata_section, (uintptr_t)__rodata_section_end},
        {(uintptr_t)__builtin_thrds, (uintptr_t)__builtin_thrds_end},
        {(uintptr_t)__builtin_devices, (uintptr_t)__builtin_devices_end},
    };

    for (usize i = 0; i < NELEM(section_ranges); i++) {
        uintptr_t start = PGROUND(section_ranges[i][0]);
        uintptr_t end   = PGROUNDUP(section_ranges[i][1]);
        usize     size  = end - start;  // x86_64_mprotect handles PGROUND internally
        const int prot  = i == 0 ? PTE_KRW : PTE_KR;

        if (start >= end) {
            // debug("Empty section [\e[93m%p\e[0m: 0B] skipped.\n", start);
            continue;
        } // skip empty sections.
        
        if (end < start) {
            debug("Invalid section: \e[93m%p\e[0m-\e[93m%p\e[0m\n", start, end);
            continue;
        }

        arch_mprotect(start, size, prot);
        // printk("%s: remmaped section: [\e[93m%p\e[0m: %5d KB]\n", strerror(err), start, B2KiB(size));
    }

    return 0;
}

int physical_memory_init(void) {
    int         err  = 0;
    uintptr_t   addr = 0;
    
    if ((err = zones_init())) {
        return 0;
    }
    
    usize size = GiB(2) - PGROUNDUP(zones[ZONEi_NORM].size);
    if ((long)size > 0) {
        addr = PGROUNDUP(zone_end(&zones[ZONEi_NORM]));
        arch_unmap_n(V2HI(addr), size);
    }

    boot_mmap_t *map = bootinfo.mmap;
    for (usize i = 0; i < bootinfo.mmapcnt; ++i) {
        addr = PGROUND(map[i].addr);
        size = PGROUNDUP(map[i].size);

        if ((V2LO(map[i].addr) < GiB(4)) && (map[i].type != MULTIBOOT_MEMORY_AVAILABLE)) {
            uint32_t flags = PTE_KRW | PTE_WTCD;
            arch_map_i(addr, V2LO(addr), size, flags);
        }
    }

    arch_map_i(bootinfo.fb.addr, V2LO(bootinfo.fb.addr),
        bootinfo.fb.size, PTE_KRW | PTE_WTCD
    );

    zones_protect_sections();

    return 0;
}