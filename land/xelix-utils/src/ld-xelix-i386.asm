; ld-xelix.asm: Trampoline functions for ld-xelix
; Copyright © 2023 Lukas Martini

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

[EXTERN resolve_callback]
[GLOBAL plt_trampoline]

plt_trampoline:
	pop edx
	pop ecx

;	push ebp
;	mov ebp, esp

	; mov eax, 0x60012f8
	; mov dword [eax], 0x6000166
	call resolve_callback

;	mov esp, ebp
;	pop ebp
	jmp eax
