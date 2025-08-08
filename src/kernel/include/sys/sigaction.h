#pragma once

#include <core/types.h>
#include <core/defs.h>
#include <sys/sigset.h>

/**
 * @brief Action to be taken when a signal arrives.
 * argument list left empty because this will be used as sa_handler or sa_sigaction. */
typedef void        (*sighandler_t)();

/*
If signo is SIGCHLD, do not generate this signal
when a child process stops (job control). This
signal is still generated, of course, when a child
terminates (but see the SA_NOCLDWAIT option
below).
*/
#define SA_NOCLDSTOP    BS(0)

/*
If signo is SIGCHLD, this option prevents the
system from creating zombie processes when
children of the calling process terminate. If it
subsequently calls wait, the calling process
blocks until all its child processes have
terminated and then returns −1 with errno set
to ECHILD.
*/
#define SA_NOCLDWAIT    BS(1)

/*
When this signal is caught, the signal is not
automatically blocked by the system while the
signal-catching function executes (unless the
signal is also included in sa_mask). Note that
this type of operation corresponds to the earlier
unreliable signals.
*/
#define SA_NODEFER      BS(2)

/*
If an alternative stack has been declared with
sigaltstack(2), this signal is delivered to the
process on the alternative stack.
*/
#define SA_ONSTACK      BS(3)

/*
The disposition for this signal is reset to
SIG_DFL, and the SA_SIGINFO flag is cleared
on entry to the signal-catching function. Note
that this type of operation corresponds to the
earlier unreliable signals. The disposition for
the two signals SIGILL and SIGTRAP can’t be
reset automatically, however. Setting this flag
can optionally cause sigaction to behave as if
SA_NODEFER is also set.
*/
#define SA_RESETHAND    BS(4)

/*
System calls interrupted by this signal are
automatically restarted.
*/
#define SA_RESTART      BS(5)

/*
This option provides additional information to a
signal handler: a pointer to a siginfo structure
and a pointer to an identifier for the process
context.
*/
#define SA_SIGINFO      BS(6)

#define MINSIGSTKSZ KiB(4)
#define SIGSTKSZ    KiB(8)

#define SS_DISABLE  1
#define SS_ONSTACK  2

typedef struct __uc_stack_t {
    void    *ss_sp;     /* stack base or pointer */
    size_t  ss_size;    /* stack size */
    i32     ss_flags;   /* flags */
} __packed uc_stack_t;

typedef struct sigaction {
    sighandler_t  sa_handler; /* addr of signal handler, SIG_IGN, or SIG_DFL */
    int             sa_flags;   /* signal options */
    sigset_t        sa_mask;    /* additional signals to block */
} sigaction_t;