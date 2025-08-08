#pragma once

#include <stdint.h>
#include <stddef.h>

#define __CAT(a, b)                     a##b

#define __unused                        __attribute__((unused))
#define __packed                        __attribute__((packed))
#define __noreturn                      __attribute__((noreturn))
#define __aligned(n)                    __attribute__((aligned(n)))
#define __fallthrough                   __attribute__((fallthrough))
#define __section(s)                    __attribute__((section(#s)))
#define __aligned_section(s, a)         __attribute__((section(#s), aligned(a)))
#define __used_section(__section__)     __attribute__((used, section(#__section__)))

#define loop()                          for (;;)
#define loop_and_yield()                for (;; sched_yield() )
#define barrier()                       ({ asm volatile ("":::"memory"); })

#define forlinked(elem, list, iter) \
    for (typeof(list) elem = list; elem; elem = iter)

#define forlinked_take(elem, list, iter, count) \
    usize __forlinked_count__ = 0;              \
    for (typeof(list) elem = list; elem && __forlinked_count__ < count; elem = iter, ++__forlinked_count__)

#define foreach(elem, list)                                             \
    for (typeof(*list) *__tmp__foreach = list,                          \
        elem = (typeof(elem))(__tmp__foreach ? *__tmp__foreach : 0); \
        elem; elem = *++__tmp__foreach)

#define foreach_reverse_take(elem, list, count)                                                 \
    for (typeof(*(list)) *__tmp_list = (list) + (count) - 1,                                    \
                         elem = (typeof(elem))(__tmp_list ? *__tmp_list : ((typeof(elem)){0})); \
         (__tmp_list >= (list)) && elem; elem = *--__tmp_list)

#define foreach_take(elem, list, count)                                                         \
    for (typeof(*(list)) *__tmp_list = (list),                                                  \
                         elem = (typeof(elem))(__tmp_list ? *__tmp_list : ((typeof(elem)){0})); \
         (__tmp_list < ((list) + (count))) && elem; elem = *++__tmp_list)

#define foreach_reverse(elem, list)                                     \
    for (typeof(*list) *__tmp__foreach = list,                          \
        elem = (typeof(elem))(__tmp__foreach ? *__tmp__foreach : 0); \
        elem; elem = *--__tmp__foreach)

#ifndef container_of
#define container_of(ptr, type, member) ({             \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
})
#endif

#define __retaddr(l)            __builtin_return_address(l)

#define BCD2binary(bcd)         ((((bcd) & 0xF0) >> 1) + (((bcd) & 0xF0) >> 3) + ((bcd) & 0xf))

#define NOT(a)                  (~(uintptr_t)(a))                   // bit-wise NOT().
#define BS(p)                   ((uintptr_t)(1) << (p))             // bit-wise SHL().
#define AND(a, b)               ((uintptr_t)(a) & (uintptr_t)(b))   // bit-wise AND().
#define OR(a, b)                ((uintptr_t)(a) | (uintptr_t)(b))   // bit-wise OR().
#define XOR(a, b)               ((uintptr_t)(a) ^ (uintptr_t)(b))   // bit-wise XOR().
#define NAND(a, b)              (AND(NOT((a)), (b)))                // bit-wise NAND().
#define SHL(a, b)               ((uintptr_t)(a) <<(uintptr_t)(b))   // bit-wise SHL().
#define SHR(a, b)               ((uintptr_t)(a) >>(uintptr_t)(b))   // bit-wise SHR().
#define BTEST(a, b)             (AND((a), BS(b)))                   // test a bit in a bitset.
#define MAX(a, b)               ((long)((a) > (b) ? (a) : (b)))     // take max of two arguments
#define MIN(a, b)               ((long)((a) < (b) ? (a) : (b)))     // take min of two arguments.
#define ABS(a)                  (((long)(a) < 0) ? -(long)(a) : (a))// take the absolute value of an argument.
#define ABSi(x)                 ({ (int)(x) < 0 ? -(int)(x) : (int)(x); })

// C('A') == Control-A
#define CTRL(x)                 ((x) - '@')

#define KiB(x)                  ((size_t)(1024ull      * ((size_t)(x))))
#define MiB(x)                  ((size_t)(KiB(1024ull) * ((size_t)(x))))
#define GiB(x)                  ((size_t)(MiB(1024ull) * ((size_t)(x))))

#define B2KiB(x)                ((size_t)(x) / KiB(1))   // convert bytes to KiB.
#define B2MiB(x)                ((size_t)(x) / MiB(1))   // convert bytes to MiB.
#define B2GiB(x)                ((size_t)(x) / GiB(1))   // convert bytes to GiB.

#define K2MiB(x)                ((size_t)(x) / KiB(1))   // convert kiB from to MiB.
#define K2GiB(x)                ((size_t)(x) / MiB(1))   // convert kiB from to GiB.

#define M2KiB(x)                ((size_t)(x) * KiB(1))   // convert MiB from to KiB.
#define M2GiB(x)                ((size_t)(x) / KiB(1))   // convert MiB from to GiB.

#define G2KiB(x)                ((size_t)(x) * MiB(1))   // convert GiB from to KiB.
#define G2MiB(x)                ((size_t)(x) * KiB(1))   // convert GiB from to MiB.

#define PGSZ                    (0x1000ull)
#define PAGESZ                  (PGSZ)
#define PGMASK                  (PGSZ - 1)
#define PAGEMASK                (PGMASK)
#define PGSZ2M                  (0x200000ull)
#define PGSZ2MASK               (PGSZ2M -1)

/* Memory alignment macros */
#define ALIGN_UP(x, align)      (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(x, align)    ((x) & ~((align) - 1))

#define __ALIGN(x, s)           (AND((uintptr_t)(x), NOT((size_t)(s) - 1)))
#define ALIGN16(x)              (AND((uintptr_t)(x), NOT(0xf)))
#define ALIGN4K(x)              (AND((uintptr_t)(x), NOT(PGMASK)))
#define PGALIGN(x)              (ALIGN4K(x))
#define OVERLAPS(x, sx, y, sy)  ((x) < ((y) + (sy)) && ((y) < ((x) + (sx))))
#define is_aligned2(p)          ((((uint64_t)(p)) & 0x01) == 0)
#define is_aligned4(p)          ((((uint64_t)(p)) & 0x03) == 0)
#define is_aligned8(p)          ((((uint64_t)(p)) & 0x07) == 0)
#define is_aligned16(p)         ((((uint64_t)(p)) & 0x0f) == 0)
#define is_aligned32(p)         ((((uint64_t)(p)) & 0x1f) == 0)
#define is_aligned64(p)         ((((uint64_t)(p)) & 0x3f) == 0)

#define MAGIC_RETADDR           (-1ul)
#define MEMMDEV                 ((uintptr_t)0xFE000000ull)
#define ismmio_addr(x)          ((((uintptr_t)(x)) >= MEMMDEV) && (((uintptr_t)(x)) < GiB(4)))

#if defined __i386__
    #define USTACK              ((uintptr_t)0xC0000000ull)
#elif defined __x86_64__
    #define USTACK              ((uintptr_t)0x800000000000ull)
#endif

#if defined __i386__
    #define UHEAP_MAX           ((uintptr_t)0x10000000ull)
#elif defined __x86_64__
    #define UHEAP_MAX           ((uintptr_t)0x40000000ull)
#endif

#if defined __i386__
    #define VMA_BASE            ((uintptr_t)0xC0000000ull)
#elif defined __x86_64__
    #define VMA_BASE            ((uintptr_t)0xFFFF800000000000ull)
#endif 

// physical address of memory mapped I/O space.
#define MEMMIO                  (0xfe000000ull)

// convert a higher-half virtual address to a lower-half virtual address.
#define V2LO(p)                 ((uintptr_t)(p) - VMA_BASE)

// convert a lower-half virtual address to a higher-half virtual address.
#define V2HI(p)                 ((uintptr_t)(p) + VMA_BASE)

#define iskernel_addr(x)        ((uintptr_t)(x) >= VMA_BASE)

// page size used for a 2MiB page
#define PGSZ2MB                 (MiB(2))
#define PGSZ1GB                 (GiB(1))
#define PGOFF(p)                (AND((uintptr_t)(p), PGMASK))
#define PG2MOFF(p)              (AND((uintptr_t)(p), PGSZ2MASK))
#define PGROUND(p)              ((uintptr_t)AND(((uintptr_t)(p)), ~PGMASK))
#define PG2MROUND(p)            ((uintptr_t)AND(((uintptr_t)(p)), ~PGSZ2MASK))
#define PGROUNDUP(p)            (PGOFF(p)   ? (PGROUND(((uintptr_t)(p)) + PAGESZ)) : (uintptr_t)(p))
#define PG2MROUNDUP(p)          (PG2MOFF(p) ? (PGROUND(((uintptr_t)(p)) + PGSZ2M)) : (uintptr_t)(p))
#define ALIGN4KUP(p)            (PGROUNDUP(p))

#define NPAGE(p)                (((size_t)(p) / PGSZ) + (PGOFF(p) ? 1 : 0))
#define N2MPAGE(p)              (((size_t)(p) / PGSZ2M) + (PG2MOFF(p) ? 1 : 0))

#define NELEM(x)                ((size_t)(sizeof ((x)) / sizeof ((x)[0])))

extern uintptr_t __kernel_start[];
extern uintptr_t __kernel_readonly_end[];
extern uintptr_t __kernel_end[];

extern uintptr_t __trampoline_section[];
extern uintptr_t __trampoline_section_end[];
extern uintptr_t __text_section[];
extern uintptr_t __text_section_end[];
extern uintptr_t __rodata_section[];
extern uintptr_t __rodata_section_end[];
