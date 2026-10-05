#include <arch/cpu.h>
#include <arch/paging.h>
#include <arch/ucontext.h>
#include <core/defs.h>
#include <core/debug.h>
#include <fs/inode.h>
#include <mm/mem.h>
#include <mm/mmap/mmap.h>
#include <mm/page.h>
#include <sys/thread.h>

#define panic_page_fault(trapframe, fault, type) ({                                                 \
    panic("%s:%d: @[\e[025453;04m0x%p\e[0m], err_code: %x : %s, from '%s' space\n",                 \
          __FILE__, __LINE__, fault->addr, fault->err_code, type, fault->user ? "user" : "kernel"); \
})

int map_anonymous_page(vmregion_t *vmregion, pagefault_desc_t *fault) {
    unsigned pte_flags = __vmregion_to_pte_flags(vmregion);
    /// Map an anonymous page (not backed by a file) into memory
    /// Map the anonymous page into the process's address space
    return arch_map_n(fault->addr, PGSZ, pte_flags);
}

int copy_page_on_write(vmregion_t *vmregion, pagefault_desc_t *fault, uintptr_t srcpaddr) {
    int err = 0;
    // virtual flags for vmregion, maskout PTE_ALLOC??
    unsigned pte_flags = __vmregion_to_pte_flags(vmregion) | (PGOFF(fault->cow->raw) & ~PTE_ALLOC);

    /// remap the page to a new location for COW
    /// vflags OR'ed with PTE_REMAPPG to force page remap.
    if ((err = arch_map_n(fault->addr, PGSZ, (PTE_REMAP | pte_flags)))) {
        return err;
    }

    // Perform the actual memory copy from the source to the destination
    if ((err = arch_memcpypv(PGROUND(fault->addr), PGROUND(srcpaddr), PGSZ))) {
        // If the copy fails, unmap the destination and restore the original COW mapping
        arch_unmap_n(fault->addr, PGSZ);
#if defined(__x86_64__)
        fault->cow->raw = srcpaddr; // Restore COW mapping
        invlpg(fault->addr);
        arch_tlbshootdown(rdcr3(), fault->addr);
        return err;
#endif
    }

    // Decrease reference count on the source page
    if ((err = __page_put(PGROUND(srcpaddr)))) {
        // If the drop the ref on page fials, unmap the destination and restore the original COW mapping
        arch_unmap_n(fault->addr, PGSZ);
#if defined(__x86_64__)
        fault->cow->raw = srcpaddr; // Restore COW mapping
        invlpg(fault->addr);
        arch_tlbshootdown(rdcr3(), fault->addr);
        return err;
#endif
    }
    return 0;
}

int enable_write_access(pagefault_desc_t *fault) {
    // Enable write access to a COW page without copying
#if defined(__x86_64__)
    fault->cow->raw |= PTE_W;
    invlpg(fault->addr);
    arch_tlbshootdown(rdcr3(), fault->addr);
#endif
    return 0;
}

int load_page_from_file(vmregion_t *vmregion, pagefault_desc_t *fault, size_t offset, usize size) {
    int         err       = 0;
    uintptr_t   paddr     = 0;
    uint8_t     buf[PGSZ] = {0};
    page_t      *page     = NULL;
    unsigned    pte_flags = __vmregion_to_pte_flags(vmregion);

    // Load a page from a file into memory
    if (vmregion->file) {
        ilock(vmregion->file);
        if (igetsize(vmregion->file) == 0) {
            iunlock(vmregion->file);
            return -EFAULT;
        }

        /**
         * @brief get the minimum size to read from the file on-disk.
         * Take into account the size between the start of the memory region and
         * the faulting address. this TODO: must be subtracted from the __vmregion_file_size(vmregion),
         * but setting size to '0' if size is greater than __vmregion_file_size(vmregion) appears to work. */
        usize min = MIN(__vmregion_file_size(vmregion) - size, igetsize(vmregion->file) - offset);
        size = (size < __vmregion_file_size(vmregion)) ? MIN(PGSZ, min) : 0;

        if (__vmregion_shared(vmregion)) { // shared vmregion?
            if ((err = icache_getpage(vmregion->file->i_cache, offset / PGSZ, &page))) {
                iunlock(vmregion->file);
                return err;
            }

            if ((err = page_get(page))) {
                iunlock(vmregion->file);
                return err;
            }

            if ((err = page_get_address(page, (void **)&paddr))) {
                iunlock(vmregion->file);
                return err;
            }

            if ((err = arch_map_i(fault->addr, paddr, PGSZ, pte_flags))) {
                iunlock(vmregion->file);
                return err;
            }
        } else { // vmregion is not shared.
            if ((err = arch_map_n(fault->addr, PGSZ, 
                pte_flags | (((__vmregion_file_size(vmregion) < __vmregion_size(vmregion)) ||
                    __vmregion_zeroed(vmregion)) ? PTE_ZERO : 0)))) {
                iunlock(vmregion->file);
                return err;
            }
            
            if ((err = iread(vmregion->file, offset, buf, size)) < 0) {
                arch_unmap_n(fault->addr, PGSZ);
                iunlock(vmregion->file);
                return err;
            }

            memcpy((void *)PGROUND(fault->addr), buf, PGSZ);
        }

        iunlock(vmregion->file);
        return 0;
    }

    return map_anonymous_page(vmregion, fault);
}

// Handle a Copy-On-Write (COW) page fault
int handle_cow_fault(vmregion_t *vmregion, pagefault_desc_t *fault) {
    int         err         = 0;
    usize       pgref       = 0;
    uintptr_t   srcpaddr    = fault->cow->raw;
    
    if ((err = __page_getcount(PGROUND(srcpaddr), &pgref)))
        return err;

    if (pgref > 1) {
        // If the page is shared, copy it before writing
        return copy_page_on_write(vmregion, fault, srcpaddr);
    } else if (pgref == 1) {
        // If the page is not shared, just mark it writable
        return enable_write_access(fault);
    } else {
        // Invalid page reference count
        return -EFAULT;
    }
}

int handle_writable_page_fault(vmregion_t *vmregion, pagefault_desc_t *fault, size_t offset, usize size) {
    int         err         = 0;
    uintptr_t   paddr       = 0;
    uint8_t     buf[PGSZ]   = {0};
    page_t      *page       = NULL;
    unsigned    pte_flags   = __vmregion_to_pte_flags(vmregion);

    // Handle writable page faults for non-COW pages
    if (fault->err_code & PTE_P) {
        debug("page fault: faulting page is already present at addr %p, access: %x\n", fault->addr, fault->err_code);
        return -EFAULT;
    }

    if (vmregion->file) {
        // Load the page from a file if it's backed by one
        ilock(vmregion->file);
        /**
         * @brief get the minimum size to read from the file on-disk.
         * Take into account the size between the start of the memory region and
         * the faulting address. this TODO: must be subtracted from the __vmregion_file_size(vmregion),
         * but setting size to '0' if size is greater than __vmregion_file_size(vmregion) appears to work.
         */
        size = (size < __vmregion_file_size(vmregion)) ? 
                (size_t)MIN(PGSZ, (size_t)MIN(__vmregion_file_size(vmregion) - size,
                igetsize(vmregion->file) - offset)) : 0;

        if (__vmregion_shared(vmregion)) { // shared vmregion?
            if ((err = icache_getpage(vmregion->file->i_cache, offset / PGSZ, &page))) {
                iunlock(vmregion->file);
                return err;
            }
            
            if ((err = page_get(page))) {
                iunlock(vmregion->file);
                return err;
            }

            if ((err = page_get_address(page, (void **)&paddr))) {
                iunlock(vmregion->file);
                return err;
            }

            if ((err = arch_map_i(fault->addr, paddr, PGSZ, pte_flags))) {
                iunlock(vmregion->file);
                return err;
            }
        } else { // vmregion is not shared.
            if ((err = arch_map_n(fault->addr, PGSZ, 
                pte_flags | (((__vmregion_file_size(vmregion) < __vmregion_size(vmregion)) ||
                    __vmregion_zeroed(vmregion)) ? PTE_ZERO : 0)))) {
                iunlock(vmregion->file);
                return err;
            }
            
            if ((err = iread(vmregion->file, offset, buf, size)) < 0) {
                arch_unmap_n(fault->addr, PGSZ);
                iunlock(vmregion->file);
                return err;
            }

            memcpy((void *)PGROUND(fault->addr), buf, PGSZ);
        }
        iunlock(vmregion->file);
        return 0;
    }

    // If the page is not backed by a file, map an anonymous page
    return map_anonymous_page(vmregion, fault);
}

int handle_write_fault(vmregion_t *vmregion, pagefault_desc_t *fault, size_t offset, usize sz) {
    // Handle a write fault
    if (!__vmregion_writable(vmregion)) {
        return -EACCES;  // Return error if the VMR is not writable
    }

    // Handle Copy-On-Write (COW) faults
    if (fault->cow && !__vmregion_shared(vmregion)) {
        return handle_cow_fault(vmregion, fault);
    }

    // Handle other writable page faults
    return handle_writable_page_fault(vmregion, fault, offset, sz);
}

int handle_read_exec_fault(vmregion_t *vmregion, pagefault_desc_t *fault, size_t offset, usize sz) {
    // Handle a read or execute fault
    if ((!__vmregion_readable(vmregion) && !__vmregion_executable(vmregion))) {
        // Invalid access: neither read nor execute is allowed, or the page is already present
        debug("Invalid access: %x, start: %p\n", vmregion->flags, __vmregion_start(vmregion));
        return -EACCES;
    }

    if (fault->err_code & PTE_P) {
        debug("page fault: faulting page is already present at addr %p, access: %x\n", fault->addr, fault->err_code);
        return -EFAULT;
    }

    // Load the page from the file if necessary
    return load_page_from_file(vmregion, fault, offset, sz);
}

int default_pgf_handler(vmregion_t *vmregion, pagefault_desc_t *fault) {
    vmregion_display(vmregion);

    // Handle the default page fault processing.
    if (vmregion == NULL || fault == NULL) {
        return -EINVAL;  // Return error if VMR or fault is invalid
    }

    const usize size = (PGROUND(fault->addr) - __vmregion_start(vmregion));

    // Calculate the offset within the file corresponding to the faulting address
    const usize offset = size + __vmregion_file_offset(vmregion);

    // debug("pagefault::size: %u, pagefault::offset: %u\n", size, offset);

    // If the fault was a write operation, handle it accordingly
    if (fault->err_code & PTE_W) {
        return handle_write_fault(vmregion, fault, offset, size);
    }

    // Otherwise, handle read/execute faults
    return handle_read_exec_fault(vmregion, fault, offset, size);
}

/// This function handles cases where the current thread is either
/// returning from a signal handler or needs to exit.
void handle_signal_or_thread_exit(mcontext_t *trapframe) {
    if (current_is_handling_signal()) {
        arch_signal_return();
    } else if (current_is_main()) {
        //this exits the entire process.
        exit(trapframe->rax);  // Exit the main thread with the provided exit code
    } else {
        thread_exit(trapframe->rax);  // Exit the current thread
    }
}

static void pagefault_send_sigsegv(mcontext_t *trapframe, pagefault_desc_t *fault) {
    // Handle a SIGSEGV signal by dumping the trapframe and panicking
    dump_tf(trapframe, 0);
    /// For now just panic here,
    /// but functionality will be added to handle this appropriately.
    panic_page_fault(trapframe, fault, "SIGSEGV");
}

static void pagefault_send_sigbus(mcontext_t *trapframe, pagefault_desc_t *fault) {
    // Handle a SIGBUS signal by dumping the trapframe and panicking
    dump_tf(trapframe, 0);
    /// For now just panic here,
    /// but functionality will be added to handle this appropriately.
    panic_page_fault(trapframe, fault, "SIGBUS");
}

static void handle_kernel_fault(mcontext_t *trapframe, pagefault_desc_t *fault) {
    // Handle page faults occurring in kernel mode
    if (fault->user) {
        // If the fault occurred in user space, send a SIGSEGV signal
        pagefault_send_sigsegv(trapframe, fault);
    } else {
        // If the fault occurred in kernel space, dump the trapframe and panic
        dump_tf(trapframe, 0);
        panic_page_fault(trapframe, fault, "undef");
    }
}

int handle_vmregion_pagefault(vmregion_t *vmregion, pagefault_desc_t *fault) {
    // If the VMR has a custom page fault handler, invoke it
    if (vmregion->vmops && vmregion->vmops->fault_handler) {
        return vmregion->vmops->fault_handler(vmregion, fault);
    } else {
        // Otherwise, use the default page fault handler
        return default_pgf_handler(vmregion, fault);
    }
}

void arch_do_page_fault(mcontext_t *trapframe) {
    // Get the faulting address and error code
    pagefault_desc_t  fault   = (pagefault_desc_t) {
        .user = 0,
        .cow = NULL,
        .page = NULL,
        .addr = rdcr2(),
        .err_code = trapframe->eno,
    };

#if defined(__x86_64__)
    // Determine if the fault occurred in user mode
    fault.user = x86_64_tf_isuser(trapframe);
    #endif
    // Check if the faulting address is a Copy-On-Write (COW) page
    int err = arch_getmapping(fault.addr, &fault.cow);

    /// TODO: increase refcnt on mmap here.
    mmap_t *mmap = current_mmap();

    if (current) {
        // Handle special cases where the trapframe's instruction pointer (RIP) indicates
        // that the thread is returning from a signal handler or should exit.
#if defined(__x86_64__)
        if (trapframe->rip == MAGIC_RETADDR) {
#endif
            handle_signal_or_thread_exit(trapframe);
        }
    }

    // Handle kernel-mode faults or cases where the mmap is NULL
    if (mmap == NULL || iskernel_addr(fault.addr)) {
        handle_kernel_fault(trapframe, &fault);
        return;
    }

    debug("fault::addr: %p, fault::errno: %d, rip: %p\n", fault.addr, fault.err_code, trapframe->rip);

    // Lock the memory map and find the corresponding virtual memory region (VMR)
    mmap_lock(mmap);

    vmregion_node_t *vmregion_node;
    err = mmap_get_address_container(mmap, fault.addr, &vmregion_node);
    if (err != 0) {
        mmap_unlock(mmap);
        // If no VMR is found, send a SIGSEGV signal to the process
        pagefault_send_sigsegv(trapframe, &fault);
        return;
    }

    // mmap_display(mmap);

    vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);
    // Handle the page fault within the found VMR
    if ((err = handle_vmregion_pagefault(vmregion, &fault)) == -EFAULT) {
        // Handle errors specific to SIGBUS or SIGSEGV signals
        pagefault_send_sigbus(trapframe, &fault);
    } else if (err) {
        pagefault_send_sigsegv(trapframe, &fault);
    }

    mmap_unlock(mmap);
}