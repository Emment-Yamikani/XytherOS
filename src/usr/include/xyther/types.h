#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

typedef     int                 pid_t;
typedef     int                 tid_t;

typedef     long                ssize_t;
typedef     unsigned            long off_t;
typedef     unsigned short      devid_t;
typedef     unsigned short      dev_t;

typedef     int                 uid_t;
typedef     int                 gid_t;
typedef     int                 ino_t;
typedef     int                 mode_t;
typedef     long                time_t;
typedef     int                 suseconds_t;
typedef     long                timer_t;
typedef     long                clock_t;
typedef     long                clockid_t;

typedef     unsigned long       nlink_t;
typedef     unsigned long       blkcnt_t;
typedef     unsigned long       blksize_t;

typedef unsigned long   useconds_t;
typedef int             pid_t;

#define FD_SETSIZE 64 /* compatibility with newlib */
typedef unsigned int fd_mask;
typedef struct _fd_set
{
    fd_mask fds_bits[1]; /* should be 64 bits */
} fd_set;

typedef     void *(*thread_entry_t)(void *);


typedef     struct timespec         timespec_t;

typedef     char                i8;
typedef     short               i16;
typedef     int                 i32;
typedef     long                i64;
typedef     long                isize;

typedef     unsigned char       u8;
typedef     unsigned short      u16;
typedef     unsigned int        u32;
typedef     unsigned long       u64;
typedef     unsigned long       usize;
typedef     unsigned long       off_t;

typedef     uint16_t            devno_t;

#define DEV_TO_MAJOR(dev)       ((devno_t)((dev) & 0xffu))
#define MAJOR_TO_DEV(major)     ((dev_t)((devno_t)(major) & 0xffu))

#define DEV_TO_MINOR(dev)       ((devno_t)(((dev) >> 8) & 0xffu))
#define MINOR_TO_DEV(minor)     ((dev_t)(((devno_t)(minor) << 8) & 0xff00u))

#define DEV_T(major, minor)     (MINOR_TO_DEV(minor) | MAJOR_TO_DEV(major))


#define __packed        __attribute__((packed))
#define __aligned(n)    __attribute__((align, n))
#define __unused        __attribute__((unused))

typedef struct meminfo_t { usize free, used; } meminfo_t;