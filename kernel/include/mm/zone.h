#pragma once

#include <boot/boot.h>
#include <core/assert.h>
#include <sync/spinlock.h>
#include <ds/queue.h>
#include <ds/bitmap.h>
#include <mm/page.h>

#define NZONE   4

typedef struct zone_t {
    usize       size;       // size of zone in bytes.
    uintptr_t   start;      // start address of this zone.
    bitmap_t    bitmap;     // zone's bitmap.
    page_t      *pages;     // array of pages.
    usize       npages;     // No. of pages in this zone.
    usize       upages;     // No. of used pages in this zone.
    u64         flags;      // zone flags.
    queue_t     queue;
    spinlock_t  lock;       // zone lock for synchronization.
} zone_t;

extern zone_t zones[NZONE];
extern const char *str_zone[];

/////////////////////////
/// zone indeices.  /////
/////////////////////////

#define ZONEi_DMA               0   // zone from 0-16MiB
#define ZONEi_NORM              1   // zone from 16MiB-2GiB
#define ZONEi_HOLE              2   // zone from 2GiB-4GiB.
#define ZONEi_HIGH              3   // zone from 4GiB and beyond.

// assert zone is valid and not a nullptr.
#define zone_assert(z)          ({ assert(z, "No zone."); })

// acquire a lock on zone.
#define zone_lock(z)            ({ zone_assert(z); spin_lock(&(z)->lock); })

// release a lock on zone.
#define zone_unlock(z)          ({ zone_assert(z); spin_unlock(&(z)->lock); })

// is this zone locked?
#define zone_islocked(z)        ({ zone_assert(z); spin_islocked(&(z)->lock); })

// ensure zone is locked before proceeding.
#define zone_assert_locked(z)   ({ zone_assert(z); spin_assert_locked(&(z)->lock); })

#define zone_assert_isnotkernel(zone, page) ({                             \
    assert(!is_kernel_addr(page_addr(page, zone)),                         \
           "Page(%p) is a builtin-kernel page.\n", page_addr(page, zone)); \
})

///////////////////////////////////////////////////
//////              zone flags.               /////
///////////////////////////////////////////////////

#define ZONE_VALID              BS(0)   // zone is valid for used.

#define zone_flags_set(z, f)    ({ zone_assert_locked(z); (z)->flags |= (f);})
#define zone_flags_mask(z, f)   ({ zone_assert_locked(z); (z)->flags &= ~(f);})
#define zone_flags_test(z, f)   ({ zone_assert_locked(z); (z)->flags & (f);})

#define zone_isvalid(z)         ({ zone_flags_test(z, ZONE_VALID); })

#define zone_size(z)            ({ zone_assert(z); (z)->size; })
#define zone_start(z)           ({ zone_assert(z); (z)->start; })
#define zone_pages(z)           ({ zone_assert(z); (z)->pages; })
#define zone_end(z)             ({ zone_assert(z); (z)->start + (z)->size; })

/// get the zone struct in which page resides.
/// on success return locked zone is ppz.
/// else an error is returned to indicate the error.
extern int getzone_bypage(page_t *page, zone_t **ppz);

/// get the zone struct in which padd resides.
/// on success return locked zone is ppz.
/// else an error is returned to indicate the error.
extern int getzone_byaddr(uintptr_t paddr, usize size, zone_t **ppz);

extern int getzone_byindex(int z_index, zone_t **ref);

static inline int getzone_index(zone_t *zone) {
    return zone - zones;
}

static inline void page_verify_watermark(page_t *page, zone_t *zone) {
    const uintptr_t addr = page_addr(page, zone);
    const page_watermark_t watermark = page_watermark(page);

    bool has_watermark = watermark == PGWM_RESERVED || watermark == PGWM_DMA_POOL ||
                         watermark == PGWM_NORM_POOL || watermark == PGWM_HOLE_POOL || watermark == PGWM_HIGH_POOL;

    assert(watermark != PGWM_RESERVED && has_watermark, "Watermark breach [%p: watermark: %p]\n", addr, watermark);
}

/// Initialize physical memory zones.
extern int zones_init(void);
