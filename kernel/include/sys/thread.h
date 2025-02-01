#pragma once

#include <arch/cpu.h>
#include <core/types.h>
#include <mm/mmap.h>

typedef struct thread_t {
    tid_t   tid;
    pid_t   pid;

    mmap_t  *t_mmap;
} thread_t;

extern tid_t  gettid(void);
extern pid_t  getpid(void);