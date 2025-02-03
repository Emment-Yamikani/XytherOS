#pragma once

#include <core/assert.h>
#include <core/types.h>
#include <fs/icache.h>
#include <mm/gfp.h>

#define MAX_PAGE_ORDER      64

typedef struct page {
    u64             flags;
    atomic_t        refcnt;
    atomic_t        mapcnt;
    icache_t        *icache;
    uintptr_t       virtual; // virtual addr
} __packed page_t;

#define PG_X            BS(0)   // page is executable.
#define PG_R            BS(1)   // page is readable.
#define PG_W            BS(2)   // page is writable.
#define PG_U            BS(3)   // page is user.
#define PG_VALID        BS(4)   // page is valid in physical.
#define PG_SHARED       BS(5)   // shared page.

#define PG_D            BS(6)   // page is dirty.
#define PG_WRITEBACK    BS(7)   // page needs writeback
#define PG_SWAPPABLE    BS(8)   // page swapping is allowed.
#define PG_SWAPPED      BS(9)   // page is swapped out.
#define PG_L            BS(10)  // page is locked in memory.
#define PG_C            BS(11)  // page is cached.

#define PG_RX           (PG_R | PG_X)
#define PG_RW           (PG_R | PG_W)
#define PG_RWX          (PG_RW| PG_X)

#define page_assert(page)           ({ assert(page, "Invalid page pointer.\n"); })

#define page_resetflags(page)       ({ (page)->flags = 0; })
#define page_testflags(page, f)     ({ (page)->flags & (f); })                       // get page flags.
#define page_setflags(page, f)      ({ (page)->flags |= (f); })
#define page_maskflags(page, f)     ({ (page)->flags &= ~(f); })

#define page_isexec(page)           ({ page_testflags(page, PG_EXEC); })     // exec'able.
#define page_iswrite(page)          ({ page_testflags(page, PG_WRITE); })    // writeable.
#define page_isread(page)           ({ page_testflags(page, PG_READ); })     // readable.
#define page_isuser(page)           ({ page_testflags(page, PG_USER); })     // user.
#define page_isvalid(page)          ({ page_testflags(page, PG_VALID); })    // valid.
#define page_isdirty(page)          ({ page_testflags(page, PG_DIRTY); })    // dirty.
#define page_isshared(page)         ({ page_testflags(page, PG_SHARED); })
#define page_iswriteback(page)      ({ page_testflags(page, PG_WRITEBACK); })
#define page_isswapped(page)        ({ page_testflags(page, PG_SWAPPED); })  // swapped.
#define page_isswappable(page)      ({ page_testflags(page, PG_SWAPPABLE); })// canswap.

#define page_setrx(page)            ({ page_setflags(page, PG_RX); })
#define page_setrw(page)            ({ page_setflags(page, PG_RW); })
#define page_setrwx(page)           ({ page_setflags(page, PG_RWX); })
#define page_setuser(page)          ({ page_setflags(page, PG_U); })          // set 'user'.
#define page_setexec(page)          ({ page_setflags(page, PG_X); })          // set 'exec'able'.
#define page_setread(page)          ({ page_setflags(page, PG_R); })          // set 'readable'.
#define page_setwrite(page)         ({ page_setflags(page, PG_W); })         // set 'writeable'.
#define page_setdirty(page)         ({ page_setflags(page, PG_D); })         // set 'dirty'.
#define page_setvalid(page)         ({ page_setflags(page, PG_VALID); })         // set 'valid'.
#define page_setshared(page)        ({ page_setflags(page, PG_SHARED); })
#define page_setwriteback(page)     ({ page_setflags(page, PG_WRITEBACK); })
#define page_setswapped(page)       ({ page_setflags(page, PG_SWAPPED); })       // set 'swapped'.
#define page_setswappable(page)     ({ page_setflags(page, PG_SWAPPABLE); })     // set 'swappable'.

#define page_maskrx(page)           ({ page_maskflags(page, PG_RX); })
#define page_maskrw(page)           ({ page_maskflags(page, PG_RW); })
#define page_maskrwx(page)          ({ page_maskflags(page, PG_RWX); })
#define page_maskuser(page)         ({ page_maskflags(page, PG_U); })          // set 'user'.
#define page_maskexec(page)         ({ page_maskflags(page, PG_X); })          // set 'exec'able'.
#define page_maskread(page)         ({ page_maskflags(page, PG_R); })          // set 'readable'.
#define page_maskwrite(page)        ({ page_maskflags(page, PG_W); })         // set 'writeable'.
#define page_maskdirty(page)        ({ page_maskflags(page, PG_D); })         // set 'dirty'.
#define page_maskvalid(page)        ({ page_maskflags(page, PG_VALID); })         // set 'valid'.
#define page_maskshared(page)       ({ page_maskflags(page, PG_SHARED); })
#define page_maskwriteback(page)    ({ page_maskflags(page, PG_WRITEBACK); })
#define page_maskswapped(page)      ({ page_maskflags(page, PG_SWAPPED); })       // set 'swapped'.
#define page_maskswappable(page)    ({ page_maskflags(page, PG_SWAPPABLE); })     // set 'swappable'.

#define page_refcnt(page)           ({ (page)->refcnt; })
#define page_virtual(page)          ({ (page)->virtual; })

#define page_index(page, zone)      ({ ((page) - (zone)->pages); })
#define page_addr(page, zone)       ({ (zone)->start + (page_index(page, zone) * PGSZ); })
#define page_end(page, npage, zone) ({ page_addr(page, zone) + (npage * PGSZ); })

usize get_page_order(usize size_in_bytes);

void page_free_n(page_t *page, usize order);
void __page_free_n(uintptr_t paddr, usize order);

void page_free(page_t *page);
void __page_free(uintptr_t paddr);

int page_alloc_n(gfp_t gfp, usize order, page_t **pp);
int __page_alloc_n(gfp_t gfp, usize order, void **pp);

int page_alloc(gfp_t gfp, page_t **pp);
int __page_alloc(gfp_t gfp, void **pp);

int page_increment(page_t *page);
int __page_increment(uintptr_t paddr);

int page_get(page_t *page);
int __page_get(uintptr_t paddr);

int page_decrement(page_t *page);
int __page_decrement(uintptr_t paddr);

void page_put(page_t *page);
int __page_put(uintptr_t paddr);

int page_get_address(page_t *page, void **ppa);

int page_getcount(page_t *page, usize *pcnt);
int __page_getcount(uintptr_t paddr, usize *pcnt);
