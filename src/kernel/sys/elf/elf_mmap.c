#include <bits/errno.h>
#include <mm/mmap/mmap.h>
#include <sys/elf/elf.h>

int elf_mmap_alloc_range(mmap_t *mmap, Elf64_Phdr *hdr, inode_t *file, uintptr_t addr, size_t memsz, vmregion_flags_t flags) {
    if (mmap == NULL || hdr == NULL || file == NULL) {
        return -EINVAL;
    }

    int err = __vmregion_check_flags(flags);
    if (err != 0) { return err; }

    const uintptr_t start = ALIGN4K(addr);
    const size_t    len   = ALIGN4KUP(memsz);

    vmregion_node_t *vmregion_node;
    if ((err = mmap_alloc_range(mmap, start, len, flags, &vmregion_node))) {
        return err;
    }

    vmregion_t *vmregion = vmregion_from_vmregion_node(vmregion_node);

    vmregion->file          = file; // TODO: increase the refcnt n file.
    vmregion->file_offset   = hdr->p_offset;
    vmregion->file_size     = hdr->p_filesz;

    return 0;
}