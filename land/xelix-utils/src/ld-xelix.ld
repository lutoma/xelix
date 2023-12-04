PHDRS {
	text PT_LOAD ;
	data PT_LOAD ;
	rodata PT_LOAD ;
}

SECTIONS {
	. = 0x500000;

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
}
