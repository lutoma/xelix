; variadic.asm: Variadic function calls
; Copyright © 2019 Lukas Martini

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

; Only works with dword arguments
[GLOBAL variadic_call]
variadic_call:
	push ebp
	mov  ebp, esp
	mov ecx, [ebp + 12] ; num_args
	mov eax, [ebp + 16] ; args
	jecxz .call

.adl:
	push dword [eax]
	add eax, 4
	dec ecx
	jnz .adl

.call:
	call [ebp + 8]
	mov esp, ebp
	pop ebp
	ret
