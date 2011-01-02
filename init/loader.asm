MBOOT_PAGE_ALIGN	equ 1
MBOOT_MEM_INFO		equ 2
MBOOT_HEADER_MAGIC	equ 0x1BADB002
MBOOT_HEADER_FLAGS	equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM		equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[BITS 32]
[GLOBAL _start]
[EXTERN kmain]

dd  MBOOT_HEADER_MAGIC
dd  MBOOT_HEADER_FLAGS
dd  MBOOT_CHECKSUM

_start:
	push ebx ; Pass arguments to kmain [ebx contains pointer to multiboot information] [see also: cdecl]
	cli
	call kmain
	cli ; Assume something really bad happened and therefore halt
	hlt
