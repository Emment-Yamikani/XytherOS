#pragma once

#include <core/defs.h>

typedef struct {
    char    *sym_name;
    void    *sym_addr;
} symbl_t;

extern symbl_t ksym_table[];
extern symbl_t ksym_table_end[];

#define EXPORT_SYMBOL(name)                                 \
symbl_t __used_section(.ksym_table) __exported_##name = {   \
    .sym_name = #name,                                      \
    .sym_addr = &name,                                      \
};
