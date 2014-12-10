; header.asm: Sets multiboot header
; Copyright © 2010-2014 Lukas Martini

; This file is part of Xelix.
;
; Xelix is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; Xelix is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with Xelix.  If not, see <http://www.gnu.org/licenses/>.

MBOOT_PAGE_ALIGN	equ 1
MBOOT_MEM_INFO		equ 2
MBOOT_HEADER_MAGIC	equ 0x1BADB002
MBOOT_HEADER_FLAGS	equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM		equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[section multiboot]
ALIGN 4
dd  MBOOT_HEADER_MAGIC
dd  MBOOT_HEADER_FLAGS
dd  MBOOT_CHECKSUM

; Reserve 4 KiB stack space
[SECTION .bss]
stack_begin:
resb 4096
stack_end:

[section .text]
EXTERN main
GLOBAL _start

_start:
	mov ebp, stack_begin
	mov esp, stack_begin
	push ebx
	push eax
	call main
