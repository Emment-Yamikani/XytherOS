#pragma once

#include <core/types.h>
#include <mm/gfp.h>

// physical memory manager entries
struct pmman {
    int         (*init)(void);    // initialize the physical memory manager.
    uintptr_t   (*alloc)(void);   // allocate a 4K page.
    void        (*free)(uintptr_t);   // free a 4K page.
    int         (*get_page)(gfp_t gfp, void **ppa);
    int         (*get_pages)(gfp_t gfp, size_t order, void **ppa);
    size_t      (*mem_used)(void); // used space (in KBs).
    size_t      (*mem_free)(void); // free space (in KBs).
};

extern struct pmman pmman;

extern int physical_memory_init(void);


// virtual memory manager
struct vmman {
    int (*init)(void);
    uintptr_t (*alloc)(size_t); // allocate an 'n'(kib), address is page aligned and size must be a multiple of 4Kib
    void (*free)(uintptr_t);    // deallocates a region of virtual memory
    size_t (*getfreesize)(void);      //returns available virtual memory space
    size_t (*getinuse)(void); // returns the size of used virtual memory space
};

extern struct vmman vmman;

extern int vmm_active(void);
extern uintptr_t mapped_alloc(size_t);
extern void mapped_free(uintptr_t, size_t);
extern void memory_usage(void);

extern int getpagesize(void);

extern void dump_free_node_list(void);
extern void dump_freevmr_list(void);
extern void dump_usedvmr_list(void);
extern void dump_all_lists(void);

typedef struct meminfo_t {
    usize      free;
    usize      used;
} meminfo_t;

#include <mm/mem_stats.h>

#include <bits/errno.h>

/**
 * put_user - Write a simple value into user space.
 * @x: Value to copy to user space.
 * @ptr: Destination address, in user space.
 *
 * Context: User context only.
 *
 * Returns zero on success, or -EFAULT on error.
 */
#define put_user(x, ptr) ({                  \
    int __ret = 0;                           \
    if (!access_ok(ptr, sizeof(*ptr))) {     \
        __ret = -EFAULT;                     \
    } else {                                 \
        user_access_begin();                 \
        *(ptr) = (x);                        \
        user_access_end();                   \
    }                                        \
    __ret;                                   \
})

/* Helper functions */
static inline bool access_ok(const void */*addr __unused*/, size_t /*size __unused*/) {
    // Check if the user space address is valid
    return true; //vmm_check_user_range((vaddr_t)addr, size, PROT_WRITE);
}

static inline void user_access_begin(void) {
    // On some architectures, might need to disable SMAP or similar
}

static inline void user_access_end(void) {
    // Re-enable protection if needed
}