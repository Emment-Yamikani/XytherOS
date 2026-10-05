#pragma once

#include <mm/mmap/mmap.h>

#include <sys/elf/elf.h>

extern int elf_mmap_alloc_range(mmap_t *mmap, Elf64_Phdr *hdr, inode_t *file, uintptr_t addr, size_t memsz, vmregion_flags_t flags);