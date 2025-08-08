#include <bits/errno.h>
#include <core/debug.h>
#include <mm/mem.h>
#include <mm/page.h>
#include <mm/zone.h>
#include <sync/atomic.h>
#include <sys/thread.h>

size_t mem_free(void);
size_t mem_used(void);
void mm_free(uintptr_t);
uintptr_t mm_alloc(void);

struct pmman pmman = {
    .free       = mm_free,
    .alloc      = mm_alloc,
    .mem_used   = mem_used,
    .mem_free   = mem_free,
    .get_page   = __page_alloc,
    .get_pages  = __page_alloc_n,
    .init       = physical_memory_init,
};

uintptr_t mm_alloc(void) {
    uintptr_t paddr = 0;
    __page_alloc(GFP_NORMAL, (void **)&paddr);
    return paddr;
}

void mm_free(uintptr_t addr) {
    __page_put(addr);
}

size_t mem_free(void) {
    size_t size = 0;
    zone_t *zone = NULL;

    for (size_t izone = ZONEi_DMA; izone < NELEM(zones); ++izone) {
        zone_lock(&zones[izone]);
        if (zone_isvalid(zone = &zones[izone])) {
            size += (zone->npages - zone->upages) * PAGESZ;
        }
        zone_unlock(&zones[izone]);
    }
    return (size / 1024);
}

size_t mem_used(void) {
    size_t size = 0;

    for (size_t zone = ZONEi_DMA; zone < NELEM(zones); ++zone) {
        zone_lock(&zones[zone]);
        if (zone_isvalid(&zones[zone]))
            size += zones[zone].upages * PAGESZ;
        zone_unlock(&zones[zone]);
    }
    return (size / 1024);
}

int page_increment(page_t *page) {
    int     err     = 0;
    zone_t  *zone   = NULL;

    if (page == NULL)
        return -EINVAL;

    if ((err = getzone_bypage(page, &zone)))
        return err;

    assert(bitmap_test(&zone->bitmap, page_index(page, zone), 1),
        "[Warning]: Increment refcnt on unallocated page(%p).\n"
        "[Advise]: Please use one of the alloc funcs.\n", page_addr(page, zone)
    );

    atomic_inc(&page->refcnt);
    zone_unlock(zone);
    return 0;
}

int page_decrement(page_t *page) {
    int     err     = 0;
    zone_t  *zone   = NULL;

    if (page == NULL)
        return -EINVAL;

    if ((err = getzone_bypage(page, &zone)))
        return err;
    
    assert(bitmap_test(&zone->bitmap, page_index(page, zone), 1),
        "[Warning]: Decrement refcnt on unallocated page(%p).\n", page_addr(page, zone)
    );
    
    if (atomic_dec_fetch(&page->refcnt) == 0) {
        assert(!(err = bitmap_unset(&zone->bitmap, page_index(page, zone), 1)),
            "[ERROR] unsetting bit(%d) page(%p).\n", page_index(page, zone), page_addr(page, zone)
        );
    }
    zone_unlock(zone);
    return 0;
}

int __page_increment(uintptr_t paddr) {
    int     err     = 0;
    usize   index   = 0;
    zone_t  *zone   = NULL;

    if (!paddr)
        return -EINVAL;

    if ((err = getzone_byaddr(paddr, PGSZ, &zone)))
        return err;

    index = (paddr - zone->start) / PGSZ;

    assert(bitmap_test(&zone->bitmap, index, 1),
        "[Warning]: Increment refcnt on unallocated page(%p).\n"
        "[Advise]: Please use one of the alloc funcs.\n", paddr
    );

    atomic_inc(&zone->pages[index].refcnt);
    zone_unlock(zone);
    return 0;
}

int __page_decrement(uintptr_t paddr) {
    int     err     = 0;
    usize   index   = 0;
    zone_t  *zone   = NULL;

    if (!paddr)
        return -EINVAL;

    if ((err = getzone_byaddr(paddr, PGSZ, &zone)))
        return err;

    index = (paddr - zone->start) / PGSZ;

    assert(bitmap_test(&zone->bitmap, index, 1),
        "[Warning]: Decrement refcnt on unallocated page(%p).\n", paddr
    );

    if (atomic_dec_fetch(&zone->pages[index].refcnt) == 0) {
        assert(!(err = bitmap_unset(&zone->bitmap, index, 1)),
            "[ERROR] unsetting bit(%d) page(%p).\n", index, paddr
        );
    }
    zone_unlock(zone);
    return 0;
}

int page_get(page_t *page) {
    return page_increment(page);
}

int __page_get(uintptr_t paddr) {
    return __page_increment(paddr);
}

void page_put(page_t *page) {
    int err = 0;

    assert((err = page_decrement(page)) == 0,
        "Failed to decrement page: %p, err: %d\n", page, err
    );
}

int __page_put(uintptr_t paddr) {
    return __page_decrement(paddr);
}

int page_get_address(page_t *page, void **ppa) {
    int     err = 0;
    zone_t  *zone  = NULL;

    if (page == NULL || ppa == NULL)
        return -EINVAL;
    
    if ((err = getzone_bypage(page, &zone)))
        return err;
    
    // convert the page to a physical address.
    *ppa = (void *)page_addr(page, zone);
    // or simply: *ppa = page->phys_addr; ?

    zone_unlock(zone);
    return 0;
}

int page_getcount(page_t *page, usize *pcnt) {
    int     err = 0;
    zone_t  *zone  = NULL;

    if (page == NULL || pcnt == NULL)
        return -EINVAL;

    if ((err = getzone_bypage(page, &zone)))
        return err;
    
    *pcnt = atomic_read(&page->refcnt);
    zone_unlock(zone);
    return 0;
}

int __page_getcount(uintptr_t paddr, usize *pcnt) {
    int     err = 0;
    zone_t  *zone   = NULL;

    if (!paddr || pcnt == NULL)
        return -EINVAL;

    if ((err = getzone_byaddr(paddr, PGSZ, &zone)))
        return err;

    *pcnt = atomic_read(&zone->pages[(paddr - zone->start) / PGSZ].refcnt);
    zone_unlock(zone);
    return 0;
}

int page_check_watermark(uintptr_t vaddr) {
    pte_t *pte;

    int err = arch_getmapping(vaddr, &pte);
    if (err) return err;

    const uintptr_t paddr = PTE2PHYS(pte);

    zone_t *zone = NULL;
    err = getzone_byaddr(paddr, PGSZ, &zone);
    if (err) return err;
    
    page_t *page = &zone->pages[(paddr - zone->start) / PGSZ];
    if (!atomic_read(&page->refcnt) || !bitmap_test(&zone->bitmap, page - zone->pages, 1)) {
        zone_unlock(zone);
        debug("Access to page handle denied.\n");;
        return -EACCES;
    }
    
    printk("%p->%p: %p\n", vaddr, paddr, page_watermark(page));

    page_verify_watermark(page, zone);
    zone_unlock(zone);

    return 0;
}