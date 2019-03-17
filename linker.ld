ENTRY(_start)
PROVIDE(tty_font = _binary_src_tty_ter_u16n_psf_start);

SECTIONS
{
	/* The first section should be at 0x100000 */
	. = 0x100000;
	PROVIDE(__kernel_start = .);

	/* The multiboot header has to be in the first 8kb */
	.text : {
		*(multiboot)
		*(.text*)
	}

	.data ALIGN(4096) : {
		*(.data)
	}

	.rodata ALIGN(4096) : {
		*(.rodata*)
	}

	.bss ALIGN(4096) : {
		*(.bss)
	}

	.eh_frame ALIGN(4096) : {
		*(.eh_frame)
	}

	PROVIDE(__kernel_end = .);
}
