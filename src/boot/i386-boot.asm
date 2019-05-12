; i386-boot.asm: Multiboot 2 header and i386 init
; Copyright © 2010-2019 Lukas Martini

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

[section .multiboot]
ALIGN 4,db 0
header_start:
	dd 0xe85250d6
	dd 0
	dd header_end - header_start

	; checksum
	dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

	; framebuffer request
	dw 5	; type
	dw 1	; flags - optional tag
	dd 24	; size
	dd 1024	; width
	dd 768	; height
	dd 32	; depth
	dd 0	; alignment filler

	; required end tag
	dw 0    ; type
	dw 0    ; flags
	dd 8    ; size
header_end:

; Reserve 16 KiB stack space
[section .bss]
GLOBAL stack_end
stack_begin:
	resb 1024 * 16
stack_end:

[section .text]
EXTERN xelix_main
GLOBAL _start

_start:
	mov ebp, stack_end
	mov esp, stack_end
	mov ecx, eax
	mov edx, ebx
	call xelix_main

.il:
	hlt
	jmp .il
	ud2
	cli
