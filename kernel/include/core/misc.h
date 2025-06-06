#pragma once

#include <core/types.h>

extern int copy_to_user(void *udst, void *ksrc, size_t size);
extern int copy_from_user(void *kdst, void * usrc, size_t size);
extern void swapi64(i64 *a0, i64 *a1);
extern void swapi32(i32 *a0, i32 *a1);
extern void swapi16(i16 *a0, i16 *a1);
extern void swapi8(i8 *a0, i8 *a1);
extern void swapbool(bool *a0, bool *a1);
extern void swapu64(u64 *a0, u64 *a1);
extern void swapu32(u32 *a0, u32 *a1);
extern void swapu16(u16 *a0, u16 *a1);
extern void swapu8(u8 *a0, u8 *a1);
extern void swapptr(void **p0, void **p1);