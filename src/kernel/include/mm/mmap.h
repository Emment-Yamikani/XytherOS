#pragma once

#include <arch/paging.h>
#include <bits/errno.h>
#include <core/types.h>
#include <core/defs.h>
#include <mm/page.h>
#include <string.h>
#include <sync/spinlock.h>

#ifndef foreach
#define foreach(elem, list) \
    for (typeof(*list) *tmp = list, elem = *tmp; elem; elem = *++tmp)
#endif //foreach

#ifndef forlinked
#define forlinked(elem, list, iter) \
    for (typeof(list) elem = list; elem; elem = iter)
#endif //forlinked

#ifndef __unused

#ifndef __attribute_maybe_unused__
#define __attribute_maybe_unused__ __attribute__ ((unused))
#endif //__attribute_maybe_unused__

#define __unused __attribute_maybe_unused__

#endif

#include <mm/vm_region.h>

#define MMAP_USER                   1

/*Page size*/
#ifndef PAGESZ
#define PAGESZ                      (0x1000)
#endif

/*Page mask*/
#ifndef PAGEMASK
#define PAGEMASK                    (0x0FFF)
#endif

/*Address Space limit (last valid address)*/
#define __mmap_limit                (USTACK - 1)

/*Returns the value smaller between 'a' and 'b'*/
#if defined MIN
    #define __min(a, b)             MIN(a, b)
#else
    #define __min(a, b)            (((a) < (b) ? (a) : (b)))
#endif

/*Is the address Page aligned?*/
#define __isaligned(p)              (((p) & PAGEMASK) == 0)

typedef struct mmap {
    int        flags;       // memory map flags.
    long       refs;        // reference count.
    void       *priv;       // private data.
    vmr_t      *arg;        // region designated for argument vector.
    vmr_t      *env;        // region designated for environment varaibles.
    vmr_t      *heap;       // region dedicated to the heap.
    uintptr_t   brk;        // brk position.
    uintptr_t   pgdir;      // page directory.
    uintptr_t   limit;      // Highest allowed address in this address space.
    size_t      guard_len;  // Size of the guard space.
    size_t      used_space; // Avalable space, may be non-contigous.
    vmr_t      *vmr_head;   // head of list of virtual memory mapping.
    vmr_t      *vmr_tail;   // tail of list of virtual memory mapping.
    spinlock_t  lock;
} mmap_t;

#define mmap_assert(mmap)           ({ assert(mmap, "No Memory Map"); })
#define mmap_lock(mmap)             ({ mmap_assert(mmap); spin_lock(&(mmap)->lock); })
#define mmap_unlock(mmap)           ({ mmap_assert(mmap); spin_unlock(&(mmap)->lock); })
#define mmap_trylock(mmap)          ({ mmap_assert(mmap); spin_trylock(&(mmap)->lock); })
#define mmap_islocked(mmap)         ({ mmap_assert(mmap); spin_islocked(&(mmap)->lock); })
#define mmap_recursive_lock(mmap)   ({ mmap_assert(mmap); spin_recursive_lock(&(mmap)->lock); })
#define mmap_assert_locked(mmap)    ({ mmap_assert(mmap); spin_assert_locked(&(mmap)->lock); })

extern int mmap_init(mmap_t *mmap);

/*Dump the Address Space, showing all holes and memory mappings*/
extern void mmap_dump_list(mmap_t);

/**
 * @brief Allocate a new locked Address Space.
 * @param &mmap_t*
 * @returns 0 on success and -ENOMEM is there are not enough resources
*/
extern int mmap_alloc(mmap_t **);

/**
 * @brief Map-in a memory region.
 * @param mmap_t *mmap
 * @param vmr_t *r
 * @returns 0 on success, or one of the errors below.
 * @retval -EINVAL if passed a NULL parameter.
 * @retval -EINVAL if 'r' is of size '0'.
 * @retval -EEXIST if part or all of 'r' is already mapped-in.
 */
extern int mmap_mapin(mmap_t *mmap, vmr_t *region);

/**
 * @brief Same as mmap_mapin()
 *  except that it begins by unmapping any region
 *  that is already mapped-in.
*/
extern int mmap_forced_mapin(mmap_t *mmap, vmr_t *region);

extern int mmap_unmap(mmap_t *mmap, uintptr_t start, size_t len);
extern int mmap_map_region(mmap_t *mmap, uintptr_t addr, size_t len, int prot, int flags, vmr_t **pvmr);

extern int mmap_contains(mmap_t *mmap, vmr_t *region);

extern int mmap_remove(mmap_t *mmap, vmr_t *region);

extern int mmap_find_stack(mmap_t *mmap, uintptr_t addr, vmr_t **pvp);
extern vmr_t *mmap_find(mmap_t *mmap, uintptr_t addr);
extern vmr_t *mmap_find_exact(mmap_t *mmap, uintptr_t start, uintptr_t end);
extern vmr_t *mmap_find_vmr_next(mmap_t *mmap, uintptr_t addr, vmr_t **pnext);
extern vmr_t *mmap_find_vmr_prev(mmap_t *mmap, uintptr_t addr, vmr_t **pprev);
extern vmr_t *mmap_find_vmr_overlap(mmap_t *mmap, uintptr_t start, uintptr_t end);

/*Begin search at the Start of the Address Space*/
#define __whence_start  0

/*Begin search at the End of the Address Space, walking backwards*/
#define __whence_end    1

extern int mmap_getholesize(mmap_t *mmap, uintptr_t addr, size_t *plen);
extern int mmap_find_hole(mmap_t *mmap, size_t len, uintptr_t *paddr, int whence);
extern int mmap_find_holeat(mmap_t *mmap, uintptr_t addr, size_t size, uintptr_t *paddr, int whence);

extern int mmap_alloc_stack(mmap_t *mmap, size_t len, vmr_t **pstack);
extern int mmap_vmr_expand(mmap_t *mmap, vmr_t *region, intptr_t incr);
extern int mmap_alloc_vmr(mmap_t *mmap, size_t len, int prot, int flags, vmr_t **pvmr);

int mmap_protect(mmap_t *mmap, uintptr_t addr, size_t len, int prot);

/// @brief 
/// @param mm 
/// @return 
extern int mmap_clean(mmap_t *mm);

/// @brief 
/// @param mm 
extern void mmap_free(mmap_t *mm);

/// @brief 
/// @param dst 
/// @param src 
/// @return 
extern int mmap_copy(mmap_t *dst, mmap_t *src);

/// @brief 
/// @param mm 
/// @param pclone 
/// @return 
extern int mmap_clone(mmap_t *mmap, mmap_t **pclone);

/**
 * @brief 
 * 
 */
extern int mmap_set_focus(mmap_t *mmap, uintptr_t *ref);

/**
 * @brief 
 * 
 * @param mmap 
 * @param argv 
 * @param envv 
 * @param pargv 
 * @param pargc 
 * @param penvv 
 * @return int 
 */
extern int mmap_argenvcpy(mmap_t *mmap, const char *argv[],
    const char *envv[], char **pargv[], int *pargc, char **penvv[]);

extern int mmap_mapin(mmap_t *mm, vmr_t *r);

/*Is the 'addr' in a hole?*/
#define __ishole(mm, addr)          (mmap_find(mm, addr) == NULL)

#define MAP_PRIVATE                 0x0001
#define MAP_SHARED                  0x0002
#define MAP_DONTEXPAND              0x0004
#define MAP_ANON                    0x0008
#define MAP_ZERO                    0x0010
#define MAP_MAPIN                   0x0020
#define MAP_LOCK                    0x0040
#define MAP_USER                    0x0080
#define MAP_GROWSDOWN               0x0100
/*region is a stack*/
#define MAP_STACK                   (MAP_GROWSDOWN)

/**
 * Map address range as given, 
 * and unmap any overlaping regions previously mapped.
*/
#define MAP_FIXED                   0x1000


#define __flags_locked(flags)       ((flags) & MAP_LOCK)
#define __flags_user(flags)         ((flags) & MAP_USER)
#define __flags_zero(flags)         ((flags) & MAP_ZERO)
#define __flags_anon(flags)         ((flags) & MAP_ANON)
#define __flags_fixed(flags)        ((flags) & MAP_FIXED)
#define __flags_stack(flags)        ((flags) & MAP_STACK)
#define __flags_mapin(flags)        ((flags) & MAP_MAPIN)
#define __flags_shared(flags)       ((flags) & MAP_SHARED)
#define __flags_private(flags)      ((flags) & MAP_PRIVATE)
#define __flags_dontexpand(flags)   ((flags) & MAP_DONTEXPAND)

#define PROT_NONE                   0x0000
#define PROT_READ                   0x0001
#define PROT_WRITE                  0x0002
#define PROT_EXEC                   0x0004

#define PROT_R PROT_READ
#define PROT_W PROT_WRITE
#define PROT_X PROT_EXEC

#define PROT_RW                     (PROT_R | PROT_W)
#define PROT_RX                     (PROT_R | PROT_X)
#define PROT_RWX                    (PROT_R | PROT_W | PROT_X)

// 
#define __prot_exec(prot)           ((prot) & PROT_EXEC)
#define __prot_read(prot)           ((prot) & PROT_READ)
#define __prot_write(prot)          ((prot) & PROT_WRITE)
#define __prot_rx(prot)             (__prot_read(prot) && __prot_exec(prot))
#define __prot_rw(prot)             (__prot_read(prot) && __prot_write(prot))
#define __prot_rwx(prot)            (__prot_rw(prot)   && __prot_exec(prot))
