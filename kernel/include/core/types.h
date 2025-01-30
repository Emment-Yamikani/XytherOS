#pragma once

#ifndef XYTHEROS_TYPES_H
#define XYTHEROS_TYPES_H

/* Fixed-width integer types */
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char        i8;
typedef signed short       i16;
typedef signed int         i32;
typedef signed long long   i64;

/* Boolean type */
typedef enum { false = 0, true = 1 } bool;

/* Size types */
typedef u64 size_t;
typedef i64 ssize_t;
typedef u64 uintptr_t;
typedef i64 intptr_t;

/* NULL pointer */
#ifndef NULL
#define NULL ((void*)0)
#endif

/* Memory alignment macros */
#define ALIGN_UP(x, align)   (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

/* Common error codes */
#define SUCCESS       0
#define ERR_INVALID  -1
#define ERR_NO_MEM   -2
#define ERR_NOT_FOUND -3

#endif /* XYTHEROS_TYPES_H */
