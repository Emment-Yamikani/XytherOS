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
    long    err     = 0;
    file_t  *file   = NULL;
    
    if (!curproc || !curproc->mmap) {
        return (void *)-EINVAL;
    }

    // printk("[%d:%d:%d]: mmap(%p, %d, %d, %d, %d, %d)\n",
        // thread_self(), getpid(), getppid(),
        // addr, len, prot, flags, fd, off
    // );
    
    // MAP_ANON can only be passed with fd == -1, return error otherwise.
    if (__flags_anon(flags) && (fd != -1)) {
        return (void *)-EINVAL;
    }

    // get the address space(mmap) struct of the current process.
    mmap_t *mmap = curproc->mmap;
    mmap_lock(mmap);
    
    vmr_t *vmr = NULL;
    if ((err = mmap_map_region(mmap, (uintptr_t)addr, len, prot, flags, &vmr))) {
        goto error;
    }

    addr = (void *)__vmr_start(vmr);

    if (__flags_anon(flags)) {
        goto anon;
    }

    /**
     * @brief Get here, the fildes (file_t *) based on the argument 'fd' passed to us.
     * fildes shall be used to make a driver specific mmap() call to map the memory
     * region to fd.
    */
    if ((err = file_get(fd, &file))) {
        goto error;
    }

    vmr->filesz  = len;
    vmr->file_pos= off;
    vmr->flags  |= VM_FILE;
    vmr->memsz   = __vmr_size(vmr);

    if ((err = fmmap(file, vmr))) {
        goto error;
        funlock(file);
    }

    funlock(file);

    goto done;
    // make an annonymous mapping.
anon:
    // if (__flags_mapin(flags)) {
    //     if ((err = paging_mappages(PGROUND(addr),
    //         ALIGN4KUP(__vmr_size(vmr)), vmr->vflags)))
    //         goto error;
    //     if (__flags_zero(flags))
    //         memset((void *)PGROUND(addr), 0,
    //             ALIGN4KUP(__vmr_size(vmr)));
    // }

    // mmap() operation is done.
done:
    mmap_unlock(mmap);
    return addr;
error:
    if (mmap && vmr) {
        mmap_remove(mmap, vmr);
    }

    if (mmap) {
        mmap_dump_list(*mmap);
    }

    if (mmap && mmap_islocked(mmap)) {
        mmap_unlock(mmap);
    }

    printk("err: %d\n", err);
    return (void *)err;
}

int munmap(void *addr, size_t len) {
    int err = -EINVAL;
    
    if (!curproc || !curproc->mmap) {
        return -EINVAL;
    }
    
    if (!__isaligned(addr)) {
        return -EINVAL;
    }
    
    mmap_t *mmap = curproc->mmap;
    mmap_lock(mmap);

    vmr_t *vmr  = mmap_find(mmap, (uintptr_t)addr);
    if (vmr == NULL) {
        goto error;
    }

    mmap_unmap(mmap, (uintptr_t)addr, PGROUND(len));
    mmap_unlock(mmap);

    return 0;
error:
    mmap_unlock(mmap);
    return err;
}

int mprotect(void *addr, size_t len, int prot) {
    if (!curproc || !curproc->mmap) {
        return -EINVAL;
    }

    mmap_t *mmap = curproc->mmap;

    mmap_lock(mmap);
    int err = mmap_protect(mmap, (uintptr_t)addr, len, prot);
    mmap_unlock(mmap);
    return err;
}