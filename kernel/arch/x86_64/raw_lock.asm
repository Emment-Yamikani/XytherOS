bits 64

section .text

global arch_raw_lock_acquire
arch_raw_lock_acquire:
    mov     rax, 1
.acquire:
    lock    xchg dword [rdi], eax
    test    eax, eax
    nop
    jnz     .acquire
    ret
;

global arch_raw_lock_release
arch_raw_lock_release:
    xor     rax, rax
    lock    xchg dword [rdi], eax
    ret
;

global arch_raw_lock_trylock
arch_raw_lock_trylock:
    mov     rax, 1
    lock    xchg dword [rdi], eax
    ret     ; return 0 if successul, otherwise if unsuccessful.
;

