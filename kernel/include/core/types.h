#pragma once

#ifndef XYTHEROS_TYPES_H
#define XYTHEROS_TYPES_H

#include <stdint.h>

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
typedef i64     isize;
typedef u64     usize;

typedef int     tid_t;
typedef int     pid_t;

typedef struct thread_t thread_t;

/* NULL pointer */
#ifndef NULL
#define NULL ((void*)0)
#endif

/* Memory alignment macros */
#define ALIGN_UP(x, align)   (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(x, align) ((x) & ~((align) - 1))

#endif /* XYTHEROS_TYPES_H */
