#include <arch/paging.h>
#include <bits/errno.h>
#include <core/debug.h>
#include <mm/zone.h>
#include <sys/thread.h>

static void do_page_free_n(page_t *page, uintptr_t addr, usize order) {
    int     err     = 0;
    usize   pos     = 0;
    zone_t  *zone   = NULL;
    usize   npage   = BS(order);

    assert(order <= 64, "Requested order(%d) is greater than 64.\n", order);

    if (page == NULL) {
        assert_eq(err = getzone_byaddr(addr, npage * PGSZ, &zone), 0,
            "Couldn't get zone by addr(%p). error: %d\n", addr, err    
        );

        page = &zone->pages[(addr - zone->start) / PGSZ];
        if (!atomic_read(&page->refcnt) ||
                !bitmap_test(&zone->bitmap, page - zone->pages, 1)) {
            assert(0, "Page(%p): couldn't take a reference to the page handle.\n", addr);
        }
    } else {
        assert(!(err = getzone_bypage(page, &zone)),
            "Couldn't get zone by page[%p]. error: %d\n", page, err
        );
    }

    zone_assert_isnotkernel(zone, page);

    if (page_end(page, npage, zone) >= zone_end(zone)) {
        debug("Out of ZONE deallocation.\nNo page was deallocated.\n");
        zone_unlock(zone);
        return;
    }

    for (pos = page - zone->pages; npage != 0; --npage, ++page, ++pos) {
        assert(atomic_read(&page->refcnt),
            "Double free detected for page[%p].\n", page_addr(page, zone)
        );

        /// page still has some references to it.
        /// so continue freeing other pages, if any.
        if (atomic_dec_fetch(&page->refcnt))
            continue;

        assert_eq(err = bitmap_unset(&zone->bitmap, pos, 1), 0,
            "Bitmap unset failed for page[%p]. error: %d\n", page_addr(page, zone) , err
        );

        page_resetflags(page);
        page_setswappable(page);

        zone->upages    -= 1;

        page->icache    = NULL;
    }

    zone_unlock(zone);
}

void page_free_n(page_t *page, usize order) {
    do_page_free_n(page, 0, order);
}

void page_free(page_t *page) {
    do_page_free_n(page, 0, 0);
}

void __page_free_n(uintptr_t addr, usize order) {
    do_page_free_n(NULL, addr, order);
}

void __page_free(uintptr_t addr) {
    __page_free_n(addr, 0);
}