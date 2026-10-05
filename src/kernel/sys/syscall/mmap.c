#include <fs/fs.h>
#include <arch/cpu.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <lib/printk.h>
#include <mm/mmap.h>
#include <bits/errno.h>
#include <sys/sysproc.h>
#include <arch/paging.h>

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off) {
    if (!curproc || !current_mmap())
        return (void *)-EINVAL;

    if (len == 0)
        return (void *)-EINVAL;

    /* MAP_ANON must not have a file descriptor */
    if ((flags & MAP_ANON) && fd != -1)
        return (void *)-EINVAL;

    mmap_t *mmap = current_mmap();
    vmregion_node_t *node = NULL;
    file_t *file = NULL;
    long err;
    
    mmap_lock(mmap);
    
    vmregion_flags_t vflags = __prot_to_vmregion_flags(prot);

    vflags |= __map_flags_to_vmregion_flags(flags);

    err = mmap_alloc_range(mmap, (uintptr_t)addr, len, vflags, &node);
    if (err) {
        goto out;
    }

    vmregion_t *vmr = vmregion_from_vmregion_node(node);
    addr = (void *)__vmregion_start(vmr);

    /* Anonymous mapping */
    if (flags & MAP_ANON)
        goto success;

    /* File-backed mapping */
    err = file_get(fd, &file);
    if (err) {
        goto out_free;
    }

    vmr->file_offset = off;
    vmr->file_size = len;
    // vmr->memoffset = 0; /* future-proof for splitting/expansion */

    err = fmmap(file, vmr);
    if (err) {
        goto out_file;
    }

success:
    mmap_unlock(mmap);
    if (file) {
        funlock(file);
    }
    return addr;

out_file:
    funlock(file);

out_free:
    mmap_free_range(mmap,
                    __vmregion_start(vmr),
                    __vmregion_size(vmr));

out:
    mmap_unlock(mmap);
    return (void *)err;
}

int munmap(void *addr, size_t len) {
    if (!curproc || !current_mmap()) {
        return -EINVAL;
    }

    if (!is_aligned4k(addr) || len == 0) {
        return -EINVAL;
    }

    mmap_t *mmap = current_mmap();
    mmap_lock(mmap);

    int err = mmap_free_range(mmap,
                              (uintptr_t)addr,
                              ALIGN4KUP(len));

    mmap_unlock(mmap);
    return err;
}

int mprotect(void *addr, size_t len, int prot) {
    if (!curproc || !current_mmap()) {
        return -EINVAL;
    }

    if (!is_aligned4k(addr) || len == 0) {
        return -EINVAL;
    }

    mmap_t *mmap = current_mmap();
    mmap_lock(mmap);

    vmregion_node_t *node;
    int err = mmap_protect_range(mmap, (uintptr_t)addr, len, prot, &node);

    mmap_unlock(mmap);
    return err;
}