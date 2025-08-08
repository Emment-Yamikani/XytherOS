#pragma once

#include <core/types.h>

typedef union sigval {
    int  sigval_int;     /* Integer value */
    void *sigval_ptr;    /* Pointer value */
} sigval_t;

typedef struct {
    int         si_signo;   /* Signal number */
    int         si_code;    /* Signal code */
    pid_t       si_pid;     /* Sending process ID */
    uid_t       si_uid;     /* Real user ID of sending process */
    void        *si_addr;   /* Address of faulting instruction */
    int         si_status;  /* Exit value or signal */
    sigval_t    si_value;   /* Signal value */
} siginfo_t;

enum {
    SIGEV_NONE,
    SIGEV_SIGNAL,
    SIGEV_THREAD,
    SIGEV_THREAD_ID,
    SIGEV_CALLBACK  // xytherOs specific, and only used in the kernel.
};

typedef struct sigevent {
    sigval_t sigev_value;  // Data passed when signal is delivered
    int sigev_signo;       // Signal number (e.g., SIGUSR1)
    int sigev_notify;      // Notification method
    union {
        tid_t _tid;        // For SIGEV_THREAD_ID
        struct {
            void (*_function)(/*sigval_t*/);
            void *_attribute;  // Thread attributes (if applicable)
        } _sigev_thread;
    } _sigev_un;
} sigevent_t;


#define sigev_tid       _sigev_un._tid
#define sigev_function  _sigev_un._sigev_thread._function
#define sigev_attribute _sigev_un._sigev_thread._attribute