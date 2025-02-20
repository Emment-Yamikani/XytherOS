section .text

global start32

extern multiboot_info_process
extern earlycons_init
extern early_init
extern setcls
extern bsp_init
extern bootothers

; Constants
PGSZ     equ 0x1000
DEVADDR  equ 0xFE000000
VMA      equ 0xFFFF800000000000
CGA_BASE equ 0xFFFF8000000B8000

[bits 32]
start32:
    cli                     ; Disable interrupts
    cld                     ; Clear direction flag (for string operations)

    ; Set up the stack
    mov     esp, (stack.top - VMA)
    mov     ebp, esp

    ; Save multiboot info (EAX = magic, EBX = multiboot info pointer)
    push    dword 0x0       ; Push a dummy value for alignment
    push    eax             ; Save multiboot magic number
    push    dword 0xFFFF8000; Push a dummy value for alignment
    push    ebx             ; Save multiboot info pointer

    ; Initialize page tables
    xor     eax, eax
    mov     edi, (_PML4_ - VMA)
    mov     cr3, edi        ; Set CR3 to point to PML4
    mov     ecx, 0x101000   ; Clear 4 pages (PML4, PDPT, PDT, PT)
    rep     stosd           ; Zero out the page tables

    ; Set up recursive mapping for PML4
    mov     edi, (_PML4_ - VMA)
    mov     eax, edi
    or      eax, 0x3        ; Set present and writable flags
    mov     dword [edi + 0xFF8], eax ; PML4E511 -> _PML4_ (recursive mapping)

    ; Map PML4E0 and PML4E256 to PDPT
    mov     eax, (PDPT - VMA)
    or      eax, 0x3        ; Set present and writable flags
    mov     dword [edi], eax        ; PML4E0 -> PDPT0
    mov     dword [edi + 0x800], eax ; PML4E256 -> PDPT0

    ; Map PDPT entries to PDT
    mov     edi, (PDPT - VMA)
    mov     eax, (PDT - VMA)
    or      eax, 0x3        ; Set present and writable flags
    mov     dword [edi], eax        ; PDPT0 -> PDT0
    add     eax, PGSZ
    mov     dword [edi + 0x8], eax  ; PDPT1 -> PDT1

    ; Map PDT entries to PT
    mov     edi, (PDT - VMA)
    mov     eax, (PT - VMA)
    or      eax, 0x3        ; Set present and writable flags
    mov     ecx, 1024       ; 1024 entries in PDT
.mapt:
    mov     dword [edi], eax        ; Map PDT entry to PT
    add     edi, 0x8
    add     eax, PGSZ
    loop    .mapt

    ; Map PT entries to physical memory (2 GiB)
    mov     edi, (PT - VMA)
    mov     eax, (0x0 | 0x3) ; Start at physical address 0, set present and writable flags
    mov     ecx, 0x80000    ; 2 GiB worth of pages (512 * 1024 entries)
.map:
    mov     dword [edi], eax        ; Map PT entry to physical memory
    add     edi, 8
    add     eax, PGSZ
    loop    .map

    ; Map device memory (MMIO)
    mov     edi, (PDPT - VMA)
    mov     eax, (DEVPDT - VMA)
    or      eax, 0x1b       ; Set present, writable, and cache-disable flags
    mov     dword [edi + 0x18], eax ; PDPT3 -> DEVPDT

    ; Map DEVPDT entries to DEVPT
    mov     edi, (DEVPDT - VMA)
    mov     eax, (DEVPT - VMA)
    or      eax, 0x1b       ; Set present, writable, and cache-disable flags
    mov     ecx, 16         ; 16 entries in DEVPDT
.mapdevpt:
    mov     dword [edi + 0xF80], eax ; Map DEVPDT entry to DEVPT
    add     edi, 8
    add     eax, PGSZ
    loop    .mapdevpt

    ; Map DEVPT entries to device memory
    mov     edi, (DEVPT - VMA)
    mov     eax, (DEVADDR | 0x1b) ; Set present, writable, and cache-disable flags
    mov     ecx, 8192       ; 8192 device pages
.mapdevs:
    mov     dword [edi], eax        ; Map DEVPT entry to device memory
    add     edi, 8
    add     eax, PGSZ
    loop    .mapdevs

.enable_longmode:
    ; Check for 64-bit support
    mov     eax, 0x80000000
    cpuid
    test    eax, 0x80000001 ; Test for extended CPU features
    jb      .noext          ; Jump if extended features are not supported

    mov     eax, 0x80000001
    cpuid
    test    edx, (1 << 29)  ; Test for long mode (64-bit) support
    jz      .no64           ; Jump if long mode is not supported

    ; Enable PAE and PSE
    mov     eax, cr4
    or      eax, (3 << 4)   ; CR4.PAE | CR4.PSE
    mov     cr4, eax

    ; Enable long mode (EFER.LM)
    mov     ecx, 0xC0000080 ; EFER MSR
    rdmsr
    or      eax, (1 << 8)   ; EFER.LM = 1
    wrmsr

    ; Enable paging
    mov     eax, cr0
    and     eax, 0x0FFFFFFF ; Disable per-CPU caching
    or      eax, 0x80000000 ; Enable PML4 paging
    mov     cr0, eax

    ; Load GDT and jump to 64-bit mode
    mov     eax, (gdtbase - VMA)
    lgdt    [eax]
    jmp     0x8:(start64 - VMA)

.noext:
    mov     eax, 0xDEADCAFE ; Error code for no extended features
    jmp     $               ; Halt

.no64:
    mov     eax, 0xDEADBEEF ; Error code for no 64-bit support
    hlt
    jmp     $               ; Halt

; GDT for 64-bit mode
gdt64:
    .null   dq 0
    .code   dq 0xAF9A000000FFFF ; Code segment descriptor
    .data   dq 0xCF92000000FFFF ; Data segment descriptor
gdtbase:
    dw  (gdtbase - gdt64) - 1   ; GDT limit
    dq  gdt64                   ; GDT base

[bits 64]
align 16
start64:
    ; Set up segment registers
    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    ; Jump to higher-half address space
    mov     rax, .high64
    jmp     rax

.high64:
    ; Set up stack and call initialization functions
    mov     rsp, (stack.top - 0x10)
    mov     rbp, stack.top

    ; Set the local storage for this core.
    mov     rdi, bspcls
    xor     rax, rax
    mov     rcx, 0x200
    rep     stosq
    mov     rdi, bspcls
    call    setcls

    ; Initialize SSE
    call    init_sse

    ; Initialize the GDT/IDT and trap vectors.
    call    bsp_init

    pop     rdi                 ; Restore multiboot info pointer
    pop     rcx                 ; Restore multiboot magic number
    ; Call kernel initialization functions
    call    multiboot_info_process

    call    bootothers          ; Start other cores.

    ; Unmap lower-half identity mapping
    mov     rdi, _PML4_
    mov     qword [rdi], 0      ; Unmap PML4E0
    invlpg  [0]                 ; Invalidate TLB entry for address 0

    call    early_init

    ; Halt the CPU (end of initialization)
    cli
    hlt

; SSE Initialization Function
global init_sse
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
    mov     rsi, no_sse_msg     ; Load error message
    call    print_message       ; Print error message
    cli                         ; Disable interrupts
    hlt                         ; Halt the CPU

.enable_sse42:
    ; SSE4.2 is supported
    call    enable_sse
    mov     rsi, sse42_supported_msg ; Load success message
    call    print_message
    ret

.enable_sse41:
    ; SSE4.1 is supported
    call    enable_sse
    mov     rsi, sse41_supported_msg
    call    print_message
    ret

.enable_ssse3:
    ; SSSE3 is supported
    call    enable_sse
    mov     rsi, ssse3_supported_msg
    call    print_message
    ret

.enable_sse3:
    ; SSE3 is supported
    call    enable_sse
    mov     rsi, sse3_supported_msg
    call    print_message
    ret

.enable_sse2:
    ; SSE2 is supported
    call    enable_sse
    mov     rsi, sse2_supported_msg
    call    print_message
    ret

.enable_sse:
    ; SSE is supported
    call    enable_sse
    mov     rsi, sse_supported_msg
    call    print_message
    ret

; Enable SSE in CR0 and CR4
enable_sse:
    fninit                   ; Initialize FPU
    mov rax, cr0
    and rax, ~(1 << 2)       ; Clear EM
    or  rax, (1 << 1)        ; Set MP
    mov cr0, rax
    
    mov rax, cr4
    or  rax, (3 << 9)        ; OSFXSR | OSXMMEXCPT
    mov cr4, rax
    ret

; Print Message Function (VGA Text Mode)
print_message:
    ; Input: RSI = pointer to null-terminated string
    ; Output: Print the string to the VGA text buffer
    xor     eax, eax
    mov     rdi, CGA_BASE       ; VGA text buffer address
    mov     ah, 0x0F            ; White text on black background
    mov     ecx, 2000
    rep     stosw
    mov     rdi, CGA_BASE       ; VGA text buffer address

.print_loop:
    lodsb                       ; Load next byte from RSI into AL
    test    al, al              ; Check for null terminator
    jz      .print_done
    stosw                       ; Write character (AL) and attribute (AH) to VGA buffer
    jmp     .print_loop
.print_done:
    ret

; Data Section for Messages
section .data
sse_supported_msg       db "SSE is supported and initialized.", 0
sse2_supported_msg      db "SSE2 is supported and initialized.", 0
sse3_supported_msg      db "SSE3 is supported and initialized.", 0
ssse3_supported_msg     db "SSSE3 is supported and initialized.", 0
sse41_supported_msg     db "SSE4.1 is supported and initialized.", 0
sse42_supported_msg     db "SSE4.2 is supported and initialized.", 0
no_sse_msg              db "Error: No SSE support detected. Halting.", 0

align 16
section .bss
align PGSZ
global _PML4_
_PML4_: resb PGSZ
PDPT:   resb PGSZ
PDT:    resb PGSZ * 2
PT:     resb PGSZ * 1024
DEVPDT: resb PGSZ
DEVPT:  resb PGSZ * 16

; Per-CPU Local storage for the Bootstrap Processor
bspcls:    resb PGSZ

stack:  resb 0x4000
.top: