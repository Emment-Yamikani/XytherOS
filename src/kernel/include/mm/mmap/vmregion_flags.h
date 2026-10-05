#pragma once

#include <stdbool.h>

typedef enum vmregion_flags_t {
    VmregionNone        = 0x0000,

    VmregionX           = 0x0001,
    VmregionW           = 0x0002,
    VmregionR           = 0x0004,
    VmregionUser        = 0x0008,

    VmregionFree        = 0x0010,
    VmregionZero        = 0x0020,
    VmregionGrowsDown   = 0x0040,
    VmregionShared      = 0x0080,
    VmregionDontExpand  = 0x0100,
    VmregionFixed       = 0x0200,
    VmregionLock        = 0x0400,
    VmregionMap         = 0x0800,
    VmregionAnon        = 0x1000,

    VmregionMask        = 0x1fff,

    VmregionRX      = VmregionR | VmregionX,
    VmregionRW      = VmregionR | VmregionW,

    VmregionData    = VmregionRW,
    VmregionRWX     = VmregionRW | VmregionX,
    VmregionText    = VmregionR  | VmregionX,
    VmregionBss     = VmregionRW | VmregionZero,
    VmregionStack   = VmregionRW | VmregionGrowsDown,
} vmregion_flags_t;

extern bool __vmregion_flags_readable(vmregion_flags_t flags);
extern bool __vmregion_flags_read_only(vmregion_flags_t flags);

extern bool __vmregion_flags_writable(vmregion_flags_t flags);
extern bool __vmregion_flags_executable(vmregion_flags_t flags);
extern bool __vmregion_flags_grows_down(vmregion_flags_t flags);

extern bool __vmregion_flags_free(vmregion_flags_t flags);
extern bool __vmregion_flags_free(vmregion_flags_t flags);

extern bool __vmregion_flags_user(vmregion_flags_t flags);
extern bool __vmregion_flags_fixed(vmregion_flags_t flags);
extern bool __vmregion_flags_zeroed(vmregion_flags_t flags);
extern bool __vmregion_flags_shared(vmregion_flags_t flags);
extern bool __vmregion_flags_dont_expand(vmregion_flags_t flags);

extern int __vmregion_check_flags(vmregion_flags_t flags);
extern int __vmregion_check_flags_for_alloc(vmregion_flags_t flags);

extern vmregion_flags_t __vmregion_flags_get_state(vmregion_flags_t from, vmregion_flags_t flags);

extern unsigned __vmregion_flags_to_pte_flags(vmregion_flags_t flags);