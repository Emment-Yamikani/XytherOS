global context_switch
;void context_switch(&&context_t, ...);
context_switch:
    push    rbp
    push    rbx
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15

                            ; rdi == &arch->t_ctx.
    mov     rax, qword[rdi] ; rax = arch->t_ctx (i.e context we're switching to).
    push    qword[rax]      ; push arch->t_ctx->link.
    mov     qword[rdi], rsp ; &arch->t_ctx = saved ctx.
    mov     rdi, rsp        ; Pass rsp as an argument to function pointed to by rip of new context.
    mov     rsp, rax        ; rsp = arch->t_ctx (i.e context we're switching to).
    add     rsp, 8          ; Skip arch->t_ctx->link.

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11    
    pop     rbx
    pop     rbp

    retq    ; pop and jmp to rip