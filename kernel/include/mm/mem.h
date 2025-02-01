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