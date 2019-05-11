ENTRY(_start)
PROVIDE(tty_font = _binary_src_tty_teru16n_psf_start);

SECTIONS
{
	. = 0x8000;
	PROVIDE(__kernel_start = .);

	.text : {
		KEEP(*(.text.boot))
		*(.text*)
	}

	.data ALIGN(4096) : {
		*(.data*)
	}

	.rodata ALIGN(4096) : {
		*(.rodata*)
	}

	/* bss start & length symbols are used in arm-boot.S to clear bss */
	.bss ALIGN(4096) : {
		PROVIDE(__bss_start = .);
		*(.bss)
	}
	PROVIDE(__bss_len = . - __bss_start);

	.eh_frame ALIGN(4096) : {
		*(.eh_frame)
	}

	PROVIDE(__kernel_end = .);
}
