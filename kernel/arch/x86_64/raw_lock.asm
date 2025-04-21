bits 64

section .text

global arch_raw_lock_init
arch_raw_lock_init:
    mov     dword [rdi], 0
    ret

global arch_raw_lock_acquire
arch_raw_lock_acquire:
    mov     rax, 1
.acquire:
    xchg    dword [rdi], eax
    test    eax, eax
    nop
    jnz     .acquire
    ret
;

global arch_raw_lock_release
arch_raw_lock_release:
    xor     rax, rax
    xchg    dword [rdi], eax
    ret
;

global arch_raw_lock_trylock
arch_raw_lock_trylock:
    mov     rax, 1
    xchg    dword [rdi], eax
    test    eax, eax
    jz      .successful
    xor     rax, rax
    ret     ; return 0 to indicate failure.
    .successful:
        mov     rax, 1
        ret ; return 1 to indicate success.
;