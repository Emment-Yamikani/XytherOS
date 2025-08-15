#pragma once

#include <arch/paging.h>
#include <core/types.h>

typedef int (*vmrops_io_handler_t)();
typedef int (*vmrops_fault_handler_t)(vm_region_t *vmr, pagefault_desc_t *vm_fault_desc);

typedef struct vmr_ops {
    vmrops_io_handler_t     io_handler;
    vmrops_fault_handler_t  fault_handler;
}vmr_ops_t;

/**
 * @brief Virtual memory region descriptor.
 * 
 */
typedef struct vmr {
    long        refs;      // No. of references to this struct.
    long        flags;     // Flags associated with this memory mapping.
    long        vflags;    // Flags used to map this region to physical memory by the paging logic.
    void        *priv;     // Private data(for module and driver-specif use).
    inode_t     *file;     // File used as backing store for this memory region.
    size_t      filesz;    // Size of this region on the file.
    size_t      memsz;     // Size of memory region as taken from program headers.
    off_t       file_pos;  // Position of this region in the file.
    mmap_t      *mmap;     // Memory map associated with this memory region.
    vmr_ops_t   *vmops;    // virtual memory operations that apply to this region.
    uintptr_t   paddr;     // physical address of this memory region(mapping).
    uintptr_t   start;     // Starting address(virtual) of this memory region.
    uintptr_t   end;       // Ending address of this memory region.
    vm_region_t *prev;     // Previous memory object in the list of memory regions.
    vm_region_t *next;     // Next memory object in the list of memory regions.
} vm_region_t, vmr_t;

#define VM_EXEC                     0x0001
#define VM_WRITE                    0x0002
#define VM_READ                     0x0004
#define VM_ZERO                     0x0008
#define VM_SHARED                   0x0010
#define VM_FILE                     0x0020
#define VM_GROWSDOWN                0x0100
#define VM_DONTEXPAND               0x0200


#define __vm_mask_exec(flags)       ((flags) &= ~VM_EXEC)
#define __vm_mask_write(flags)      ((flags) &= ~VM_WRITE)
#define __vm_mask_read(flags)       ((flags) &= ~VM_READ)
#define __vm_mask_rwx(flags)        ((flags) &= ~(VM_READ | VM_WRITE | VM_EXEC))

/*Executable*/
#define __vm_exec(flags)            ((flags) & VM_EXEC)

/*Writable*/
#define __vm_write(flags)           ((flags) & VM_WRITE)

/*Readable*/
#define __vm_read(flags)            ((flags) & VM_READ)

/* Read-only */
#define __vm_ronly(flags)           (__vm_read(flags) && !__vm_write(flags) && !__vm_exec(flags))

#define __vm_wonly(flags)           ((__vm_write(flags) && !__vm_read(flags) && !__vm_exec(flags)))

/*Readable or Executable*/
#define __vm_rx(flags)              (__vm_read(flags) && __vm_exec(flags))

/*Readable or Writable*/
#define __vm_rw(flags)              (__vm_read(flags) && __vm_write(flags))

/*Readable, Writable or Executable*/
#define __vm_rwx(flags)             (__vm_rw(flags) && __vm_exec(flags))

#define __vm_zero(flags)            ((flags) & VM_ZERO)

#define __vm_filebacked(flags)      ((flags) & VM_FILE)

/*Sharable*/
#define __vm_shared(flags)          ((flags) & VM_SHARED)

/*Do not expand*/
#define __vm_dontexpand(flags)      ((flags) & VM_DONTEXPAND)

/*Can be expanded*/
#define __vm_can_expand(flags)      (!__vm_dontexpand(flags))

/*Expansion edge grows downwards*/
#define __vm_growsdown(flags)       ((flags) & VM_GROWSDOWN)

/*Expansion edge grows upwards*/
#define __vm_growsup(flags)         (!__vm_growsdown(flags))

#define __valid_addr(addr)          ((addr) <= __mmap_limit)

static inline void __vmr_mask_exec(vm_region_t *vmr) {
    __vm_mask_exec(vmr->flags);
}

static inline void __vmr_mask_write(vm_region_t *vmr) {
    __vm_mask_write(vmr->flags);
}

static inline void __vmr_mask_read(vm_region_t *vmr) {
    __vm_mask_read(vmr->flags);
}

static inline void __vmr_mask_rwx(vm_region_t *vmr) {
    __vm_mask_rwx(vmr->flags);
}

static inline bool __vmr_exec(vm_region_t *vmr) {
    return __vm_exec(vmr->flags) ? true : false;
}

static inline bool __vmr_write(vm_region_t *vmr) {
    return __vm_write(vmr->flags) ? true : false;
}

static inline bool __vmr_read(vm_region_t *vmr) {
    return __vm_read(vmr->flags) ? true : false;
}

static inline bool __vmr_read_only(vm_region_t *vmr) {
    return __vm_ronly(vmr->flags) ? true : false;
}

static inline bool __vmr_write_only(vm_region_t *vmr) {
    return __vm_wonly(vmr->flags) ? true : false;
}

static inline bool __vmr_rx(vm_region_t *vmr) {
    return __vm_rx(vmr->flags) ? true : false;
}

static inline bool __vmr_rw(vm_region_t *vmr) {
    return __vm_rw(vmr->flags) ? true : false;
}

static inline bool __vmr_rwx(vm_region_t *vmr) {
    return __vm_rwx(vmr->flags) ? true : false;
}

static inline bool __vmr_shared(vm_region_t *vmr) {
    return __vm_shared(vmr->flags) ? true : false;
}

static inline bool __vmr_zero(vm_region_t *vmr) {
    return __vm_zero(vmr->flags) ? true : false;
}

static inline bool __vmr_filebacked(vm_region_t *vmr) {
    return (vmr->file && __vm_filebacked(vmr->flags)) ? true : false;
}

static inline bool __vmr_dontexpand(vm_region_t *vmr) {
    return __vm_dontexpand(vmr->flags) ? true : false;
}

static inline bool __vmr_can_expand(vm_region_t *vmr) {
    return __vm_can_expand(vmr->flags) ? true : false;
}

static inline bool __vmr_growsdown(vm_region_t *vmr) {
    return __vm_growsdown(vmr->flags) ? true : false;
}

static inline bool __vmr_growsup(vm_region_t *vmr) {
    return __vm_growsup(vmr->flags) ? true : false;
}

/*Is memory region a stack?*/
static inline bool __vmr_isstack(vm_region_t *vmr) {
    return __vmr_growsdown(vmr);
}


/*Size of a memory mapping*/
static inline size_t __vmr_size(vm_region_t *vmr)   {
    return ((vmr->end - vmr->start) + 1);
}

/*size of region in file on-disk*/
static inline size_t __vmr_filesz(vm_region_t *vmr) {
    return vmr->filesz;
}

static inline size_t __vmr_memsz(vm_region_t *vmr)  {
    return vmr->memsz;
}

/**Upper edge of a memory mapping.
 **NB*: 'edge' is not part of the mapping*/
static inline uintptr_t __vmr_upper_bound(vm_region_t *vmr)    {
    return (vmr->end + 1);
}

/**Lower edge of a memory mapping.
 **NB*: 'edge' is not part of the mapping*/
static inline uintptr_t __vmr_lower_bound(vm_region_t *vmr)    {
    return (vmr->start - 1);
}

/*Next mapping in the list*/
static inline vm_region_t *__vmr_next(vm_region_t *vmr)   {
    return vmr->next;
}

/*Previous mapping in the list*/
static inline vm_region_t *__vmr_prev(vm_region_t *vmr)   {
    return vmr->prev;
}

static inline uintptr_t __vmr_start(vm_region_t *vmr)  {
    return vmr->start;
}
static inline uintptr_t __vmr_end(vm_region_t *vmr)    {
    return vmr->end;
}

static inline off_t __vmr_filepos(vm_region_t *vmr)    {
    return vmr->file_pos;
}

static inline long __vmr_vflags(vm_region_t *vmr) {
    return vmr->vflags;
}

extern int  vmr_alloc(vmr_t **);
extern void vmr_free(vmr_t *region);
extern void vmr_dump(vmr_t *region, int index);
extern int  vmr_copy(vmr_t *rdst, vmr_t *rsrc);
extern int  vmr_clone(vmr_t *src, vmr_t **pclone);
extern int  vmr_split(vmr_t *region, uintptr_t addr, vmr_t **pvmr);