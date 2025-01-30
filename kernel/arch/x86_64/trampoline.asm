[bits 16]

section .trampoline

; Segment Descriptor Flags
SEG_ACC     EQU (1 << 0)  ; Accessed bit
SEG_RW      EQU (1 << 1)  ; Readable/Writeable bit
SEG_DC      EQU (1 << 2)  ; Direction/Conforming bit
SEG_CODE    EQU (1 << 3)  ; Executable bit (code segment)

SEG_TYPE    EQU (1 << 4)  ; Descriptor type (0 = system, 1 = code/data)
SEG_DPL0    EQU (0 << 5)  ; Descriptor privilege level 0
SEG_PRES    EQU (1 << 7)  ; Present bit

SEG_LONG    EQU (1 << 5)  ; Long mode (64-bit) segment
SEG_DB      EQU (1 << 6)  ; Default operation size (0 = 16-bit, 1 = 32-bit)
SEG_GRAN    EQU (1 << 7)  ; Granularity (0 = 1B, 1 = 4KB)

align 4
global ap_trampoline
ap_trampoline:
    cli                     ; Disable interrupts
    cld                     ; Clear direction flag (for string operations)

    ; Enable PAE (Physical Address Extension)
    mov     eax, cr4
    or      eax, (1 << 5)   ; Set CR4.PAE
    mov     cr4, eax

    ; Enable Long Mode (EFER.LME)
    mov     ecx, 0xC0000080 ; EFER MSR
    rdmsr
    or      eax, (1 << 8)   ; Set EFER.LME
    wrmsr

    ; Load PML4 (Page Map Level 4)
    mov     eax, dword [PML4]
    mov     cr3, eax        ; Set CR3 to PML4 base address

    ; Load GDT
    lgdt    [gdt64.pointer]

    ; Enable Paging and Protection
    mov     eax, cr0
    and     eax, 0x0FFFFFFF ; Disable per-CPU caching
    or      eax, 0x80000001 ; Enable paging (CR0.PG) and protection (CR0.PE)
    mov     cr0, eax

    ; Far jump to 64-bit mode
    jmp     gdt64.code:long_mode

; GDT for 64-bit mode
gdt64:
    .null: equ $ - gdt64
        dq 0                  ; Null descriptor
    .code: equ $ - gdt64
        dw 0xFFFF             ; Limit low
        dw 0                  ; Base low
        db 0                  ; Base mid
        db (SEG_PRES | SEG_DPL0 | SEG_TYPE | SEG_CODE | SEG_RW) ; Flags
        db (SEG_GRAN | SEG_LONG | 0xF) ; Flags
        db 0                  ; Base high
    .data: equ $ - gdt64
        dw 0xFFFF             ; Limit low
        dw 0                  ; Base low
        db 0                  ; Base mid
        db (SEG_PRES | SEG_DPL0 | SEG_TYPE | SEG_RW) ; Flags
        db (SEG_GRAN | SEG_DB | 0xF) ; Flags
        db 0                  ; Base high
    .pointer:
        dw $ - gdt64 - 1      ; GDT limit
        dq gdt64              ; GDT base

[bits 64]

long_mode:
    ; Set up segment registers
    mov     ax, 0x10          ; Data segment selector
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    ; Set up stack
    mov     rsp, qword [stack] ; Load AP bootstrap stack
    mov     rbp, rsp           ; Set base pointer

    ; Initialize SSE
    call    init_sse

    ; Jump to AP entry point
    mov     rax, qword [entry] ; Load AP entry point
    call    rax                ; Call AP entry function

    ; Halt the CPU (end of AP execution)
    cli
    hlt
    jmp     $

; SSE Initialization Function
init_sse:
    ; Check CPU capabilities using CPUID
    mov     eax, 1              ; Set EAX to 1 to request feature flags
    cpuid                       ; Execute CPUID instruction

    ; Check for SSE4.2 (ECX bit 20)
    test    ecx, 1 << 20
    jnz     .enable_sse42

    ; Check for SSE4.1 (ECX bit 19)
    test    ecx, 1 << 19
    jnz     .enable_sse41

    ; Check for SSSE3 (ECX bit 9)
    test    ecx, 1 << 9
    jnz     .enable_ssse3

    ; Check for SSE3 (ECX bit 0)
    test    ecx, 1 << 0
    jnz     .enable_sse3

    ; Check for SSE2 (EDX bit 26)
    test    edx, 1 << 26
    jnz     .enable_sse2

    ; Check for SSE (EDX bit 25)
    test    edx, 1 << 25
    jnz     .enable_sse

    ; No SSE support, halt the system
    cli                         ; Disable interrupts
    hlt                         ; Halt the CPU

.enable_sse42:
    ; SSE4.2 is supported
    call    enable_sse
    ret

.enable_sse41:
    ; SSE4.1 is supported
    call    enable_sse
    ret

.enable_ssse3:
    ; SSSE3 is supported
    call    enable_sse
    ret

.enable_sse3:
    ; SSE3 is supported
    call    enable_sse
    ret

.enable_sse2:
    ; SSE2 is supported
    call    enable_sse
    ret

.enable_sse:
    ; SSE is supported
    call    enable_sse
    ret

; Enable SSE in CR0 and CR4
enable_sse:
    ; Enable SSE in CR0 (Clear EM bit, Set MP bit)
    mov     rax, cr0
    and     rax, ~(1 << 2)      ; Clear EM bit (bit 2)
    or      rax, 1 << 1         ; Set MP bit (bit 1)
    mov     cr0, rax

    ; Enable SSE in CR4 (Set OSFXSR and OSXMMEXCPT bits)
    mov     rax, cr4
    or      rax, (1 << 9)       ; Set OSFXSR bit (bit 9)
    or      rax, (1 << 10)      ; Set OSXMMEXCPT bit (bit 10)
    mov     cr4, rax

    ret

; Padding to ensure the trampoline code fits within 4024 bytes
times 4024 - ($ - $$) db 0

; Data for AP initialization
PML4:  dq 0  ; Level 4 Page Map
stack: dq 0  ; AP bootstrap stack
entry: dq 0  ; AP entry point