section .text

[bits 64]

; Macros for ISR and IRQ definitions
%macro ISR_NOERR 1
global isr%1
isr%1:
    push    qword 0            ; Push dummy error code
    push    qword %1           ; Push interrupt number
    jmp     stub               ; Jump to common handler
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    push    qword %1           ; Push interrupt number (error code is already pushed by CPU)
    jmp     stub               ; Jump to common handler
%endmacro

%macro IRQ 2
global irq%1
irq%1:
    push    qword 0            ; Push dummy error code
    push    qword %2           ; Push IRQ number
    jmp     stub               ; Jump to common handler
%endmacro

; Define ISRs (0-31)
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31
ISR_NOERR 128                  ; Syscall interrupt
ISR_NOERR 129                  ; panic interrupt


; Define IRQs (0-31, mapped to interrupts 32-63)
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47
IRQ 16, 48
IRQ 17, 49
IRQ 18, 50
IRQ 19, 51
IRQ 20, 52
IRQ 21, 53
IRQ 22, 54
IRQ 23, 55
IRQ 24, 56
IRQ 25, 57
IRQ 26, 58
IRQ 27, 59
IRQ 28, 60
IRQ 29, 61
IRQ 30, 62
IRQ 31, 63
IRQ 32, 64
IRQ 33, 65

; Common handler for ISRs and IRQs
global trapret
extern trap

; Save CPU context
%macro save_mctx  0
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    rdi
    push    rsi
    push    rbp

    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rax, cr2
    push    rax

    mov     rax, ds
    push    rax

    push    fs

    sub     rsp, 56 ; Account for reserved space in mcontext_t
%endmacro

; Restore CPU context
%macro rstor_mctx 0
    add     rsp, 56 ; Account for reserved space in mcontext_t

    pop     fs

    pop     rax
    mov     ds, ax

    add     rsp, 8 ; pop cr2

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8

    pop     rbp
    pop     rsi
    pop     rdi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax
%endmacro

; Common stub for ISRs and IRQs
stub:
    ; swapgs                      ; Swap GS base (if needed for user-space handling)
    save_mctx                   ; Save CPU registers

    ; Reserve space for ucontext_t struct (uc_link, uc_sigmask, uc_stack)
    sub     rsp, 48

    ; Call the trap handler (C function)
    mov     rdi, rsp            ; Pass the stack pointer as the first argument
    call    trap

    ; Clean up the stack
    add     rsp, 48             ; Remove reserved space for ucontext_t

trapret:
    rstor_mctx                  ; Restore CPU ritrs
    ; swapgs                      ; Restore GS base (if needed)
    add     rsp, 16             ; Remove interrupt number and error code
    iretq                       ; Return from interrupt

; Interrupt simulator (for testing)
global __simulate_trap
__simulate_trap:
    mov     rax, [rsp]          ; Get the return address (RIP)
    push    rax                 ; Push return address (RIP)
    push    qword 0x10          ; Push data segment selector (DS)
    push    rsp                 ; Push stack pointer (SS:SP)
    pushfq                      ; Push flags register (RFLAGS)
    cli
    push    qword 0x8           ; Push code segment selector (CS)
    push    rax                 ; Push return address (RIP)
    push    qword 0             ; Push dummy error code
    push    qword 160           ; Push 160 as trapno
    jmp     stub                ; Jump to common handler