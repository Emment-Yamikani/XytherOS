[bits 16]
section .trampoline

VMA equ 0xffff800000000000

; Segment Descriptor Flags (adjusted for correct bit positions)
SEG_ACC     equ (1 << 0)   ; Accessed
SEG_RW      equ (1 << 1)   ; Readable/Writeable
SEG_DC      equ (1 << 2)   ; Direction/Conforming
SEG_CODE    equ (1 << 3)   ; Code segment
SEG_TYPE    equ (1 << 4)   ; Descriptor type (code/data)
SEG_PRES    equ (1 << 7)   ; Present
SEG_LONG    equ (1 << 5)   ; Long mode (bit 53 in descriptor)
SEG_GRAN    equ (1 << 7)   ; Granularity (page vs byte)

align 4
global trampoline
trampoline:
    cli
    cld

    ; Enable PAE
    mov eax, cr4
    or  eax, (1 << 5)       ; CR4.PAE
    mov cr4, eax

    ; Enable Long Mode
    mov ecx, 0xC0000080     ; EFER MSR
    rdmsr
    or  eax, (1 << 8)       ; EFER.LME
    wrmsr

    ; Load PML4 (full 64-bit physical address)
    mov eax, dword [PML4]
    mov cr3, eax           ; CR3 still takes 32-bit in PAE, but we assume PML4 <4GB

    ; Load GDT
    lgdt [gdt64.pointer]

    ; Enable protected mode and paging
    mov eax, cr0
    and eax, 0x0FFFFFFF
    or  eax, 0x80000001     ; CR0.PG | CR0.PE
    mov cr0, eax

    ; Far jump to 64-bit mode
    jmp gdt64.code:long_mode

[bits 64]
long_mode:
    ; Update segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Set up stack using PHYSICAL address
    mov rsp, qword [stack]
    mov rbp, rsp

    ; Initialize SSE
    call init_sse

    mov rax, .high64
    mov rdx, VMA
    add rax, rdx
    jmp  rax
.high64:
    ; Jump to entry point
    mov rax, qword [entry]
    call rax

    cli
    hlt
;
init_sse:
    fninit                   ; Initialize FPU
    mov rax, cr0
    and rax, ~(1 << 2)       ; Clear CR0.EM
    or  rax, (1 << 1)         ; Set CR0.MP
    mov cr0, rax
    
    mov rax, cr4
    or  rax, (3 << 9)         ; CR4.OSFXSR | CR4.OSXMMEXCPT
    mov cr4, rax
    ret

; GDT with physical address awareness
gdt64:
    .null: equ $ - gdt64
        dq 0
    .code: equ $ - gdt64
        dw 0xFFFF           ; Limit (ignored)
        dw 0                ; Base (low)
        db 0                ; Base (mid)
        db SEG_PRES | SEG_TYPE | SEG_CODE | SEG_RW  ; 0x9A
        db SEG_GRAN | SEG_LONG | 0xF ; 0xAF
        db 0                ; Base (high)
    .data: equ $ - gdt64
        dw 0xFFFF
        dw 0
        db 0
        db SEG_PRES | SEG_TYPE | SEG_RW ; 0x92
        db SEG_GRAN | 0xF   ; 0x8F
        db 0
    .pointer:
        dw $ - gdt64 - 1
        dq gdt64

; Padding and data section
times 4024 - ($ - $$) db 0

; Data area (must be filled by boot code)
PML4:  dq 0
stack: dq 0
entry: dq 0