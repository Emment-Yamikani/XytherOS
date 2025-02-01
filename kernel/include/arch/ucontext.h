#pragma once

#include <core/types.h>
#include <core/defs.h>
#include <sys/_signal.h>
#include <arch/x86_64/mmu.h>

#define x86_64_tf_isuser(tf)    ({((tf)->errno & 0x4) ? 1 : 0; })

typedef struct __context_t context_t;
typedef struct __context_t {
    context_t *link;
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 rbx;
    u64 rbp;
    u64 rip;
} context_t;

// No. of registers in caller-callee context.
#define NREGCTX ((sizeof (context_t)) / sizeof (uintptr_t))

enum {
    CXT_LINK,
#define CTX_LINK CTX_LINK
    CTX_R15, 
#define CTX_R15 CTX_R15
    CTX_R14, 
#define CTX_R14 CTX_R14
    CTX_R13, 
#define CTX_R13 CTX_R13
    CTX_R12, 
#define CTX_R12 CTX_R12
    CTX_R11, 
#define CTX_R11 CTX_R11
    CTX_RBX, 
#define CTX_RBX CTX_RBX
    CTX_RBP, 
#define CTX_RBP CTX_RBP
    CTX_RIP, 
#define CTX_RIP CTX_RIP
};

extern void trapret(void);
extern void dump_ctx(context_t *ctx, int halt);
extern void swtch(context_t **old, context_t *new);
extern void context_switch(context_t **pcontext);

/*machine context*/
typedef struct __mcontext_t {
    // general purpose registers.
#if defined (__x86_64__)
    u64 fs;
    u64 ds;

    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;

    u64 rbp;
    u64 rsi;
    u64 rdi;
    u64 rdx;
    u64 rcx;
    u64 rbx;
    u64 rax;

    // TODO: include cr2 and other necessary registers.

    u64 trapno;
    u64 errno;

    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
#endif // #if defined (__x86_64__)
} mcontext_t/*Machine context*/;

typedef struct __ucontext_t {
    ucontext_t *uc_link;    /* pointer to context resumed when */
                            /* this context returns */
    sigset_t    uc_sigmask; /* signals blocked when this context */
                            /* is active */
    uc_stack_t  uc_stack;   /* stack used by this context */
    i32         uc_resvd;
    i64         uc_flags;   /* flags*/
    mcontext_t  uc_mcontext;/* machine-specific representation of */
                            /* saved context */
} ucontext_t;

#define uctx_isuser(ctx)    ({ ((ctx)->uc_mcontext.cs == (((SEG_UCODE64 << 3) | DPL_USR))) ? 1 : 0; })