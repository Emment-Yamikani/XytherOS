#pragma once

/**
 * @brief Atomic Operations
 * @author Banda Yamikani Emment
 *
 * This header provides a set of macros for atomic operations.
 * Functions prefixed with `atomic_` require passing pointers rather than direct variables.
 */

typedef _Atomic(char)   atomic_char;
typedef _Atomic(short)  atomic_short;
typedef _Atomic(int)    atomic_int;
typedef _Atomic(long)   atomic_long;

typedef _Atomic(unsigned char)  atomic_uchar;
typedef _Atomic(unsigned short) atomic_ushort;
typedef _Atomic(unsigned int)   atomic_uint;
typedef _Atomic(unsigned long)  atomic_ulong;


typedef atomic_char     atomic_i8;
typedef atomic_short    atomic_i16;
typedef atomic_int      atomic_i32;
typedef atomic_long     atomic_i64;

typedef atomic_uchar    atomic_u8;
typedef atomic_ushort   atomic_u16;
typedef atomic_uint     atomic_u32;
typedef atomic_ulong    atomic_u64;

typedef atomic_u64      atomic_t;

#define compiler_barrier()  asm volatile("" ::: "memory")

#define memory_barrier()    __sync_synchronize()

// Atomic read and write operations
#define atomic_read(ptr)                __atomic_load_n((ptr), __ATOMIC_SEQ_CST)
#define atomic_write(ptr, val)          __atomic_store_n((ptr), (val), __ATOMIC_SEQ_CST)

// Atomic clear (used with 'char' or 'bool')
#define atomic_clear(ptr)               __atomic_clear((ptr), __ATOMIC_SEQ_CST)

// Atomic test and set (used with 'char' or 'bool')
// Returns 'true' if the previous value was 'true'
#define atomic_test_and_set(ptr)        __atomic_test_and_set((ptr), __ATOMIC_SEQ_CST)

// Atomic arithmetic operations

#define atomic_fetch_sub(ptr, val)      __atomic_fetch_sub((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_sub_fetch(ptr, val)      __atomic_sub_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_sub(ptr, val)            atomic_fetch_sub(ptr, val)

#define atomic_fetch_add(ptr, val)      __atomic_fetch_add((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_add_fetch(ptr, val)      __atomic_add_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_add(ptr, val)            atomic_fetch_add(ptr, val)

#define atomic_fetch_and(ptr, val)      __atomic_fetch_and((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_and_fetch(ptr, val)      __atomic_and_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_and(ptr, val)            atomic_fetch_and(ptr, val)

#define atomic_fetch_xor(ptr, val)      __atomic_fetch_xor((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_xor_fetch(ptr, val)      __atomic_xor_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_xor(ptr, val)            atomic_fetch_xor(ptr, val)

#define atomic_fetch_or(ptr, val)       __atomic_fetch_or((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_or_fetch(ptr, val)       __atomic_or_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_or(ptr, val)             atomic_fetch_or(ptr, val)

#define atomic_fetch_nand(ptr, val)     __atomic_fetch_nand((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_nand_fetch(ptr, val)     __atomic_nand_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_nand(ptr, val)           atomic_fetch_nand(ptr, val)

// Atomic increment and decrement operations
#define atomic_fetch_inc(ptr)           atomic_fetch_add((ptr), 1)
#define atomic_inc_fetch(ptr)           atomic_add_fetch((ptr), 1)
#define atomic_inc(ptr)                 atomic_fetch_inc(ptr)

#define atomic_fetch_dec(ptr)           atomic_fetch_sub((ptr), 1)
#define atomic_dec_fetch(ptr)           atomic_sub_fetch((ptr), 1)
#define atomic_dec(ptr)                 atomic_fetch_dec(ptr)

// Atomic exchange operations
#define atomic_xchg(ptr, val)           __atomic_exchange_n((ptr), (val), __ATOMIC_SEQ_CST)

// Atomic compare and exchange
// Returns 'true' if the operation was successful (i.e., the exchange was performed)
#define atomic_cmpxchg(ptr, __exp__, __des__)  __atomic_compare_exchange_n((ptr), (__exp__), (__des__), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
