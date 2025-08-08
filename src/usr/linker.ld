ENTRY(start)
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

USER_LMA = 0x4000000;

SECTIONS {
	. = USER_LMA;

	.text BLOCK(4K) : ALIGN(4K) {
		*(.text)
	}

	.rodata BLOCK(4K) : ALIGN(4K) {
		*(.rodata)
	}

	.init_array : {
		PROVIDE_HIDDEN (__init_array_start = .);
		*(.init_array);
		PROVIDE_HIDDEN (__init_array_end = .);
	}

	.data BLOCK(4K) : ALIGN(4K) {
		*(.data)
	}

	.bss BLOCK(4K) : ALIGN(4K) {
		*(COMMON)
		*(.bss)
	}

	.eh_frame BLOCK(4K) : ALIGN(4K) {
		*(.eh_frame)
	}

	/DISCARD/ : {
		*(.comment)
		*(.note.gnu.build-id)
	}
}
