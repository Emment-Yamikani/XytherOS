#pragma once

#ifndef XYTHEROS_TYPES_H
#define XYTHEROS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef     unsigned char           uchar;
typedef     unsigned short          ushort;
typedef     unsigned int            uint;
typedef     unsigned long           ulong;

typedef     signed char             schar;
typedef     signed short            sshort;
typedef     signed int              sint;
typedef     signed long             slong;

typedef     uchar                   u8;
typedef     ushort                  u16;
typedef     uint                    u32;
typedef     ulong                   u64;

typedef     char                    i8;
typedef     short                   i16;
typedef     int                     i32;
typedef     long                    i64;

typedef     ulong                   off_t;
typedef     long                    isize;
typedef     ulong                   usize;

typedef     float                   f32;
typedef     double                  f64;

typedef     uchar                   flags8_t;    // 8-bit flags.
typedef     ushort                  flags16_t;   // 16-bit flags.
typedef     uint                    flags32_t;   // 32-bit flags.
typedef     ulong                   flags64_t;   // 64-bit flags.

typedef     int                     pid_t;
typedef     int                     tid_t;

typedef     int                     uid_t;
typedef     int                     gid_t;
typedef     int                     ino_t;
typedef     int                     mode_t;


typedef     long                    time_t;
typedef     long                    timer_t;
typedef     long                    clock_t;
typedef     long                    clockid_t;
typedef     int                     suseconds_t;
typedef     struct timeval          timeval_t;
typedef     struct timespec         timespec_t;


typedef     uint16_t                dev_t;
typedef     slong                   ssize_t;

typedef     struct inode*           INODE;
typedef     struct inode            inode_t;
typedef     struct __pipe_t         pipe_t;

typedef     int                     devno_t;

typedef struct devid {
    inode_t *inode;
    devno_t major; // major number.
    devno_t minor; // minor number.
    int     type;  // type of device.
} devid_t;

typedef     struct device           device_t;

typedef     struct cpu_t            cpu_t;

typedef     struct thread_t         thread_t;

/**
 * @brief Process information structure */
typedef     struct __proc_t         proc_t;

typedef     struct vmr              vmr_t;
typedef     struct page             page_t;
typedef     struct vm_fault_t       vm_fault_t;

typedef     struct queue            queue_t;

typedef     enum   tstate_t         tstate_t;
typedef     void*                   (*thread_entry_t)();
typedef     struct __arch_thread_t  arch_thread_t;

/*          Signal related          */

typedef     struct __ucontext_t     ucontext_t;
typedef     struct __sig_stack_t    sig_stack_t;

typedef     struct meminfo_t        meminfo_t;

#endif /* XYTHEROS_TYPES_H */
