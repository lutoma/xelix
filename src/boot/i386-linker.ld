ENTRY(_start)
tty_font = _binary_src_tty_teru16n_psf_start;

PHDRS {
	headers PT_PHDR PHDRS ;
	multiboot PT_LOAD FILEHDR PHDRS;
	text PT_LOAD ;
	data PT_LOAD ;
	rodata PT_LOAD ;
	ul_visible PT_LOAD ;
}

SECTIONS {
	/* The first section should be at 0x100000 */
	. = 0x100000;
	__kernel_start = .;

	/* The multiboot header has to be in the first 8kb */
	.multiboot : {
		*(.multiboot)
	} :multiboot

	.text ALIGN(4096) : {
		*(.text)
	} :text

	.data ALIGN(4096) : {
		*(.data)
	} :data

	.bss ALIGN(4096) : {
		*(.bss)
	} :data

	.rodata ALIGN(4096) : {
		*(.rodata*)
	} :rodata

	.eh_frame ALIGN(4096) : {
		*(.eh_frame)
	} :rodata

	/* Userland visible kernel functions and data */
	.text.ul_visible ALIGN(4096) : {
		__ul_visible_start = .;
		*(.text.ul_visible)
	} :ul_visible

	.bss.ul_visible ALIGN(4096) : {
		*(.bss.ul_visible)
	} :ul_visible

	__ul_visible_end = .;
	__kernel_end = .;
}
