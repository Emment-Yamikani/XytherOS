#include <bits/errno.h>
#include <core/defs.h>
#include <fs/fs.h>
#include <mm/kalloc.h>
#include <mm/mmap.h>
#include <string.h>
#include <sys/binary_loader.h>
#include <sys/elf/elf.h>

// Global symbol table for dynamic linking (can be extended with a hash map for efficiency).
typedef struct symtab_entry {
    char *name;
    void *addr;
} symtab_entry_t;

#define MAX_SYMBOLS     1024
static int              symb_cnt = 0;
static symtab_entry_t   symb_tab[MAX_SYMBOLS];

#define foreach_elf_segment(elf_phdr_table, phdr_num) for (Elf64_Phdr *phdr = &((elf_phdr_table)[0]); phdr < &((elf_phdr_table)[(phdr_num)]); phdr++)

int elf_check(inode_t *binary) {
    Elf64_Ehdr h;

    if (binary == NULL) {
        return -EINVAL;
    }

    iassert_locked(binary);
    if (iread(binary, 0, &h, sizeof h) != sizeof h) {
        return -EAGAIN;
    }

    if ((h.e_ident[EI_MAG0] != 0x7f) || (h.e_ident[EI_MAG1] != 'E')||
        (h.e_ident[EI_MAG2] != 'L')  || (h.e_ident[EI_MAG3] != 'F')) {
        return -EINVAL;
    }

    if (h.e_ident[EI_CLASS]   != ELFCLASS64  ||
        h.e_ident[EI_DATA]    != ELFDATA2LSB ||
        h.e_ident[EI_VERSION] != EV_CURRENT) {
        return -EINVAL;
    }

    return 0;
}

// Register a symbol in the global symbol table.
int register_symbol(const char *name, void *addr) {
    if (symb_cnt >= MAX_SYMBOLS) {
        return -ENOMEM;
    }

    if (!(symb_tab[symb_cnt].name = strdup(name))) {
        return -ENOMEM;
    }

    symb_tab[symb_cnt].addr = addr;
    symb_cnt++;
    return 0;
}

// Lookup a symbol in the global symbol table.
void *resolve_symbol(const char *name) {
    for (int i = 0; i < symb_cnt; i++) {
        if (string_eq(symb_tab[i].name, name)) {
            return symb_tab[i].addr;
        }
    }

    return NULL;
}

// ELF Loader for Executables and Shared Libraries.
int elf_loader(inode_t *binary, mmap_t *mmap) {
    int         err         = 0;
    u64         memsz       = 0;
    u64         rela_count  = 0;
    Elf64_Ehdr   elf_hdr    = {0};
    Elf64_Dyn   *elf_dyn    = NULL;
    Elf64_Rela  *elf_rela   = NULL;
    Elf64_Sym   *elf_symtab = NULL;
    char        *strtab     = NULL;

    if (binary == NULL) {
        return -EINVAL;
    }

    iassert_locked(binary);
    mmap_assert_locked(mmap);

    // Read ELF Header.
    if ((err = iread(binary, 0, &elf_hdr, sizeof elf_hdr)) != sizeof elf_hdr) {
        return err;
    }

    // Allocate and read program headers.
    Elf64_Phdr *elf_phdr_table = (Elf64_Phdr *)kcalloc(elf_hdr.e_phnum, elf_hdr.e_phentsize);
    if (elf_phdr_table == NULL) { return -ENOMEM; }

    if ((err = iread(binary, elf_hdr.e_phoff, elf_phdr_table, elf_hdr.e_phentsize * elf_hdr.e_phnum))
        != (elf_hdr.e_phentsize * elf_hdr.e_phnum)) {
        printk("%s:%d: Failed to read program headers.\n", __FILE__, __LINE__);
        goto error;
    }

    // Load segments.
    // for (u64 i = 0; i < elf_hdr.e_phnum; ++i) {
        // Elf64_Phdr  *phdr = &elf_phdr_table[i];
    foreach_elf_segment(elf_phdr_table, elf_hdr.e_phnum) {
        if (phdr->p_type == PT_LOAD) {

            // printk("elf_phdr_table[%d]: [%s] addr: %p, off: %p, memsz: %ld, file_size: %ld\n",
            //     phdr-elf_phdr_table, (phdr->p_flags & 7) == 7 ? "rwx" :
            //     (phdr->p_flags & 7) == 6 ? "rw_" : (phdr->p_flags & 7) == 5 ? "r_x" :
            //     (phdr->p_flags & 7) == 4 ? "r__" : (phdr->p_flags & 7) == 3 ? "_wx" :
            //     (phdr->p_flags & 7) == 2 ? "_w_" : (phdr->p_flags & 7) == 1 ? "__x" : "___",
            //     phdr->p_vaddr, phdr->p_offset, phdr->p_memsz, phdr->p_filesz
            // );

            memsz    = PGROUNDUP(phdr->p_memsz);

            int prot =  (phdr->p_flags & PF_X ? PROT_X : 0) |
                        (phdr->p_flags & PF_W ? PROT_W : 0) |
                        (phdr->p_flags & PF_R ? PROT_R : 0);

            vmregion_flags_t flags = __prot_to_vmregion_flags(prot);

            flags |= VmregionFixed | VmregionDontExpand;

            if ((err = elf_mmap_alloc_range(mmap, phdr, binary, phdr->p_vaddr, memsz, flags))) {
                printk("%s:%d: Failed to map region[%p: %d]. err: %d\n", __FILE__, __LINE__, phdr->p_vaddr, memsz, err);
                goto error;
            }
        } else if (phdr->p_type == PT_DYNAMIC) { // Allocate and read the dynamic section.
            u64 dyn_size = phdr->p_filesz;

            if (!(elf_dyn = kmalloc(dyn_size))) {
                err = -ENOMEM;
                goto error;
            }

            if (iread(binary, phdr->p_offset, elf_dyn, dyn_size) != (isize)dyn_size) {
                err = -EIO;
                goto error;
            }
        }
    }

    // mmap_dump_list(*proc->mmap);

    // Process dynamic section.
    if (elf_dyn) {
        for (u64 i = 0; elf_dyn[i].d_tag != DT_NULL; i++) {
            switch (elf_dyn[i].d_tag) {
            case DT_SYMTAB:
                elf_symtab      = (Elf64_Sym *)(elf_hdr.e_entry + elf_dyn[i].d_un.d_ptr);
                break;
            case DT_STRTAB:
                strtab      = (char *)(elf_hdr.e_entry + elf_dyn[i].d_un.d_ptr);
                break;
            case DT_RELA:
                elf_rela        = (Elf64_Rela *)(elf_hdr.e_entry + elf_dyn[i].d_un.d_ptr);
                break;
            case DT_RELASZ:
                rela_count  = elf_dyn[i].d_un.d_val / sizeof(Elf64_Rela);
                break;
            }
        }
    }

    // Apply relocations.
    if (elf_rela && elf_symtab && strtab) {
        for (u64 i = 0; i < rela_count; i++) {
            void        *sym_addr   = NULL;
            Elf64_Rela  *rel        = &elf_rela[i];
            Elf64_Sym   *sym        = &elf_symtab[ELF64_R_SYM(rel->r_info)];
            void        *rel_addr   = (void *)(elf_hdr.e_entry + rel->r_offset);
            const char  *sym_name   = strtab + sym->st_name;

            if (!(sym_addr = resolve_symbol(sym_name))) {
                printk("Unresolved symbol: %s\n", sym_name);
                err = -EINVAL;
                goto error;
            }

            switch (ELF64_R_TYPE(rel->r_info)) {
            case R_X86_64_RELATIVE:
                *(Elf64_Addr *)rel_addr = elf_hdr.e_entry + rel->r_addend;
                break;
            case R_X86_64_GLOB_DAT:
            case R_X86_64_JUMP_SLOT:
                *(Elf64_Addr *)rel_addr = (Elf64_Addr)sym_addr;
                break;
            default:
                printk("Unsupported relocation type: %d\n", ELF64_R_TYPE(rel->r_info));
                err = -EINVAL;
                goto error;
            }
        }
    }

    kfree(elf_dyn);
    kfree(elf_phdr_table);

    mmap->entry = (thread_entry_t)elf_hdr.e_entry;

    return 0;
error:
    if (elf_phdr_table) {
        kfree(elf_phdr_table);
    }

    if (elf_dyn) {
        kfree(elf_dyn);
    }

    printk("error: %d occurred while trying to load ELF file\n", err);
    return err;
}

BINARY_LOADER(elf_hdr, elf_check, elf_loader);