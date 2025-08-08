#pragma once

#include <arch/x86_64/mmu.h>
#include <core/types.h>
#include <core/defs.h>
#include <sys/_signal.h>

#define x86_64_tf_isuser(tf)    ({((tf)->eno & 0x4) ? 1 : 0; })

typedef struct __context_t context_t;
struct __context_t {
    context_t *link;
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 rbx;
    u64 rbp;
    u64 rip;
};

extern void trapret(void);
extern void dump_ctx(context_t *ctx, int halt);
extern void context_switch(context_t **pcontext, ...);

typedef struct __fpxreg {
    ushort  significand[4];
    ushort  exponent;
    ushort  __reserved1[3];
} __fpxreg_t;

typedef struct __xmmreg {
    uint    element[4];
} __xmmreg_t;

/* Structure to describe FPU registers.  */
typedef struct __fpstate {
    /* 64-bit FXSAVE format.  */
    ushort      cwd;
    ushort      swd;
    ushort      ftw;
    ushort      fop;
    ulong       rip;
    ulong       rdp;
    uint        mxcsr;
    uint        mxcr_mask;
    __fpxreg_t  _st[8];
    __xmmreg_t  _xmm[16];
    uint        __reserved1[24];
} __fpstate_t;

/*machine context*/
typedef struct __mcontext_t {
    // general purpose registers.
#if defined (__x86_64__)
    __fpstate_t *fpstate;

    u64 _[6]; // TODO: include other necessary registers.

    u64 fs;

    u64 ds;

    u64 cr2;

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

    u64 trapno;
    u64 eno;

    u64 rip;
    u64 cs;
    u64 rfl;
    u64 rsp;
    u64 ss;
#endif // #if defined (__x86_64__)
} __packed mcontext_t/*Machine context*/;

typedef struct __ucontext_t {
    ucontext_t *uc_link;    /* pointer to context resumed when this context returns */
    sigset_t    uc_sigmask; /* signals blocked when this context is active */
    uc_stack_t  uc_stack;   /* stack used by this context */
    i32         uc_padding;
    i64         uc_flags;   /* flags*/
    __fpstate_t uc_fpstate; /* FPU state */
    mcontext_t  uc_mcontext;/* machine-specific representation of saved context */
} ucontext_t;

#define uctx_isuser(ctx)    ({ ((ctx)->uc_mcontext.cs == (((SEG_UCODE64 << 3) | DPL_USR))) ? 1 : 0; })
