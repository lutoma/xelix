ENTRY(_start)

SECTIONS
{
	/* The first section should be at 0x100000 */
	. = 0x100000;

	kernel_start = .;

	/* The multiboot header has to be in the first 8kb */
	.text : {
		*(multiboot)
		*(.text)
	}

	.data ALIGN(4096) : {
		*(.data)
	}

	.rodata ALIGN(4096) : {
		*(.rodata)
	}

	.bss ALIGN(4096) : {
		*(.bss)
	}
}
