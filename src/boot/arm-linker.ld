ENTRY(_start)
PROVIDE(tty_font = _binary_src_tty_teru16n_psf_start);

SECTIONS
{
	. = 0;
	PROVIDE(__kernel_start = .);

	.init : {
		KEEP(*(.init))
	}

	. = 0x8000;
	.text : {
		KEEP(*(.text.boot))
		*(.text*)
	}

	.data ALIGN(4096) : {
		*(.data)
	}

	.rodata ALIGN(4096) : {
		*(.rodata*)
	}

	PROVIDE(__bss_start = .);
	.bss ALIGN(4096) : {
		*(.bss)
	}
	PROVIDE(__bss_end = .);

	.eh_frame ALIGN(4096) : {
		*(.eh_frame)
	}

	PROVIDE(__kernel_end = .);
}
