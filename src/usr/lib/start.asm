bits 64

section .text

extern main
extern exit

global start
start:
    call main

    mov  rdi, rax
    call exit
    jmp $

section .rodata

section .data

section .bss