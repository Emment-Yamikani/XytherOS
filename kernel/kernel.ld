ENTRY(start32)
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

TRAMPOLINE = 0x8000;
KERNEL_LMA = 0x1000000;
KERNEL_VMA = 0xFFFF800000000000;


SECTIONS {
    . = TRAMPOLINE;
    .trampoline ALIGN (4K) : AT (ADDR (.trampoline)) {
        __trampoline_section = .;
        KEEP(*(.trampoline))
        KEEP(*(.multiboot))
        KEEP(*(.init32))
        __trampoline_section_end = .;
    }

    . = KERNEL_LMA + KERNEL_VMA;

    __kernel_start = .;
    .text ALIGN (4K) : AT (ADDR (.text) - KERNEL_VMA) {
        __text_section = .;
        *(.text)
        __text_section_end = .;
    }

    .rodata ALIGN (4K) : AT (ADDR (.rodata) - KERNEL_VMA) {
        __rodata_section = .;
        *(.rodata)
        __rodata_section_end = .;
    }

    .__builtin_thrds ALIGN (4K) : AT(ADDR(.__builtin_thrds) - KERNEL_VMA) {
        __builtin_thrds = .;
        KEEP(*(.__builtin_thrds*))
        __builtin_thrds_end = .;
    }

    .__builtin_devices ALIGN (4K) : AT(ADDR(.__builtin_devices) - KERNEL_VMA) {
        __builtin_devices = .;
        KEEP(*(.__builtin_devices*))
        __builtin_devices_end = .;
    }

    __kernel_readonly_end = .;

    .data ALIGN (4K) : AT (ADDR (.data) - KERNEL_VMA) {
        *(.data)
    }

    .bss ALIGN (4K) : AT (ADDR (.bss) - KERNEL_VMA) {
        *(.bss)
        *(COMMON)
    }

    /DISCARD/ : {
      *(.comment)
      /* Exception handling section - we do not need it*/
      *(.eh_frame)
    }

    __kernel_end = .;
}