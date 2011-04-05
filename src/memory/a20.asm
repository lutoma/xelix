; a20.asm: Checks if the a20 line is enabled
; Copyright © 2011 Lukas Martini

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
; along with Xelix. If not, see <http://www.gnu.org/licenses/>.

; Originally released into the public domain. Taken from the OSdev wiki
; (http://wiki.osdev.org/A20_Line).

[bits 16]
[GLOBAL a20_check]
 
; Function: check_a20
;
; Purpose: to check the status of the a20 line in a completely
; self-contained state-preserving way.
;
; Returns: 0 in ax if the a20 line is disabled (memory wraps around)
;		   1 in ax if the a20 line is enabled (memory does not wrap around)

a20_check:
	pushf
	push ds
	push es
	push di
	push si
 
	cli
 
	xor ax, ax ; ax = 0
	mov es, ax
 
	not ax ; ax = 0xFFFF
	mov ds, ax
 
	mov di, 0x0500
	mov si, 0x0510
 
	mov al, byte [es:di]
	push ax
 
	mov al, byte [ds:si]
	push ax
 
	mov byte [es:di], 0x00
	mov byte [ds:si], 0xFF
 
	cmp byte [es:di], 0xFF
 
	pop ax
	mov byte [ds:si], al
 
	pop ax
	mov byte [es:di], al
 
	mov ax, 0
	je check_exit
 
	mov ax, 1
 
check_exit:
	pop si
	pop di
	pop es
	pop ds
	popf
 
	ret
