[bits 64]
section .text

global rdtsc
global rdtscp
rdtscp:
    mov     eax, 0x80000001
    cpuid
    test    edx, (1 << 27)
    xor     rcx, rcx
    jz      .rdtsc
    rdtscp
    jmp     .done
    .rdtsc:
        rdtsc
    .done:
        mov dword [rdi], ecx
        push    rax
        mov     dword [rsp + 4], edx
        pop     qword rax
        ret

rdtsc:
    lfence
    rdtsc
    push    rax
    mov     dword [rsp + 4], edx
    pop     qword rax
    ret

global rdrax
rdrax:
    ret

global rdrflags
rdrflags:
    pushfq
    pop     qword rax
    ret

global wrrflags
wrrflags:
    push    qword rdi
    popfq
    ret

global rdcr0
rdcr0:
    mov     rax, cr0
    ret

global wrcr0
wrcr0:
    mov     cr0, rdi
    ret

global rdcr2
rdcr2:
    mov     rax, cr2
    ret

global rdcr3
rdcr3:
    mov     rax, cr3
    ret

global wrcr3
wrcr3:
    mov     cr3, rdi
    ret

global rdcr4
rdcr4:
    mov     rax, cr4
    ret

global wrcr4
wrcr4:
    mov     cr4, rdi
    ret

global cache_disable
cache_disable:
    mov     rax, cr0
    and     eax, 0x8000000f
    mov     cr0, rax
    ret

global wrmsr
wrmsr:
    mov     rcx, rdi
    mov     rax, rsi
    mov     rdx, rax
    shr     rdx, 32
    wrmsr
    ret

global rdmsr
rdmsr:
    mov     rcx, rdi
    rdmsr
    shl     rdx, 32
    or      rdx, rax
    xchg    rdx, rax
    ret

global loadgdt64 ; loadgdt64(gdtptr, cs, gs, ss)
loadgdt64:
    lgdt    [rdi]
    push    qword rcx
    push    qword rsp
    pushfq
    push    qword rsi
    mov     rax, qword .switch
    push    qword rax
    iretq
    .switch:
        mov     rax, rcx
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     ss, ax
        mov     rax, rdx
        mov     gs, ax
        add     rsp, 8
    ret

global loadidt
loadidt:
    lidt    [rdi]
    ret

global loadtr
loadtr:
    ltr     di
    ret

global invlpg
invlpg:
    invlpg  [rdi]
    ret

global wrxcr
wrxcr:
    mov     rcx, rdi
    mov     rax, rsi
    mov     rdx, rsi
    shr     rdx, 32
    xsetbv
    ret

global rdxcr
rdxcr:
    mov     rcx, rdi
    xgetbv
    or      rdx, rax
    xchg    rax, rdx 
    ret

global fxsave
fxsave:
    fxsave64 [rdi]
    ret

global fxrstor
fxrstor:
    fxrstor64 [rdi]
    ret

global rdrsp
rdrsp:
    mov     rax, rsp
    ret

global rdrbp
rdrbp:
    mov     rax, rbp
    ret

global fninit
; C prototype: void fninit(void);
fninit:
    fninit
    ret

global cpu_pause
; C prototype: void cpu_pause(void);
cpu_pause:
    pause
    ret

global outb
; C prototype: void outb(uint16_t port, uint8_t data);
outb:
    mov dx, di
    mov ax, si
    out dx, al
    ret

global inb
; C prototype: uint8_t inb(uint16_t port);
inb:
    mov dx, di
    in  al, dx
    ret

global outw
; C prototype: void outw(uint16_t port, uint16_t data);
outw:
    mov dx, di
    mov ax, si
    out dx, ax
    ret

global inw
; C prototype: uint16_t inw(uint16_t port);
inw:
    mov dx, di
    in  ax, dx
    ret

global outd
; C prototype: void outd(uint16_t port, uint32_t data);
outd:
    mov dx, di
    mov eax, esi
    out dx, eax
    ret

global ind
; C prototype: uint32_t ind(uint16_t port);
ind:
    mov dx, di
    in  eax, dx
    ret