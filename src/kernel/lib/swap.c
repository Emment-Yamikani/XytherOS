#include <bits/errno.h>
#include <core/assert.h>
#include <string.h>
#include <sync/atomic.h>

int copy_to_user(void *udst, void *ksrc, size_t size) {
    if (udst == NULL || ksrc == NULL)
        return -EFAULT;
    if (size == 0)
        return -ERANGE;

    if (memcpy(udst, ksrc, size) != udst)
        return -EINVAL;
    return 0;
}

int copy_from_user(void *kdst, void * usrc, size_t size) {
    if (kdst == NULL || usrc == NULL)
        return -EFAULT;

    if (size == 0)
        return -ERANGE;

    if (memcpy(kdst, usrc, size) != kdst)
        return -EINVAL;

    return 0;
}

void swapi64(i64 *a0, i64 *a1) {
    atomic_exchange_memory(a0, a1);
}

void swapi32(i32 *a0, i32 *a1) {
    atomic_exchange_memory(a0, a1);
}

void swapi16(i16 *a0, i16 *a1) {
    atomic_exchange_memory(a0, a1);
}

void swapi8(i8 *a0, i8 *a1) {
    atomic_exchange_memory(a0, a1);
}

void swapbool(bool *a0, bool *a1) {
    atomic_exchange_memory(a0, a1);
}

void swapu64(u64 *a0, u64 *a1) {
    atomic_exchange_memory(a0, a1);
}

void swapu32(u32 *a0, u32 *a1) {
    atomic_exchange_memory(a0, a1);
}

void swapu16(u16 *a0, u16 *a1) {
    atomic_exchange_memory(a0, a1);
}

void swapu8(u8 *a0, u8 *a1) {
    atomic_exchange_memory(a0, a1);
}

void swapptr(void **a0, void **a1) {
    atomic_exchange_memory(a0, a1);
}