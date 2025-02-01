#pragma once

/**
 * @brief Atomic Operations
 * @author Banda Yamikani Emment
 *
 * This header provides a set of macros for atomic operations.
 * Functions prefixed with `atomic_` require passing pointers rather than direct variables.
 */

typedef unsigned long atomic_t;

#define compiler_barrier()  asm volatile("" ::: "memory")

#define memory_barrier()    __sync_synchronize()

// Atomic read and write operations
#define atomic_read(ptr)               ({ __atomic_load_n((ptr), __ATOMIC_SEQ_CST); })
#define atomic_write(ptr, val)         ({ __atomic_store_n((ptr), (val), __ATOMIC_SEQ_CST); })

// Atomic clear (used with 'char' or 'bool')
#define atomic_clear(ptr)              ({ __atomic_clear((ptr), __ATOMIC_SEQ_CST); })

// Atomic test and set (used with 'char' or 'bool')
// Returns 'true' if the previous value was 'true'
#define atomic_test_and_set(ptr)       ({ __atomic_test_and_set((ptr), __ATOMIC_SEQ_CST); })

// Atomic arithmetic operations

#define atomic_fetch_sub(ptr, val)     ({ __atomic_fetch_sub((ptr), (val), __ATOMIC_SEQ_CST); })
#define atomic_sub_fetch(ptr, val)     ({ __atomic_sub_fetch((ptr), (val), __ATOMIC_SEQ_CST); })

#define atomic_fetch_add(ptr, val)     ({ __atomic_fetch_add((ptr), (val), __ATOMIC_SEQ_CST); })
#define atomic_add_fetch(ptr, val)     ({ __atomic_add_fetch((ptr), (val), __ATOMIC_SEQ_CST); })

#define atomic_fetch_and(ptr, val)     ({ __atomic_fetch_and((ptr), (val), __ATOMIC_SEQ_CST); })
#define atomic_and_fetch(ptr, val)     ({ __atomic_and_fetch((ptr), (val), __ATOMIC_SEQ_CST); })

#define atomic_fetch_xor(ptr, val)     ({ __atomic_fetch_xor((ptr), (val), __ATOMIC_SEQ_CST); })
#define atomic_xor_fetch(ptr, val)     ({ __atomic_xor_fetch((ptr), (val), __ATOMIC_SEQ_CST); })

#define atomic_fetch_or(ptr, val)      ({ __atomic_fetch_or((ptr), (val), __ATOMIC_SEQ_CST); })
#define atomic_or_fetch(ptr, val)      ({ __atomic_or_fetch((ptr), (val), __ATOMIC_SEQ_CST); })

#define atomic_fetch_nand(ptr, val)    ({ __atomic_fetch_nand((ptr), (val), __ATOMIC_SEQ_CST); })
#define atomic_nand_fetch(ptr, val)    ({ __atomic_nand_fetch((ptr), (val), __ATOMIC_SEQ_CST); })

// Atomic increment and decrement operations
#define atomic_fetch_inc(ptr)          ({ atomic_fetch_add((ptr), 1); })
#define atomic_inc_fetch(ptr)          ({ atomic_add_fetch((ptr), 1); })
#define atomic_inc(ptr)                ({ atomic_fetch_inc(ptr); })

#define atomic_fetch_dec(ptr)          ({ atomic_fetch_sub((ptr), 1); })
#define atomic_dec_fetch(ptr)          ({ atomic_sub_fetch((ptr), 1); })
#define atomic_dec(ptr)                ({ atomic_fetch_dec(ptr); })

// Atomic exchange operations
#define atomic_xchg(ptr, val)          ({ __atomic_exchange_n((ptr), (val), __ATOMIC_SEQ_CST); })

// Atomic compare and exchange
// Returns 'true' if the operation was successful (i.e., the exchange was performed)
#define atomic_cmpxchg(ptr, __exp__, __des__)  ({ __atomic_compare_exchange_n((ptr), (__exp__), (__des__), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); })
