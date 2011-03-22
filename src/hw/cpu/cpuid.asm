; cpuid.asm: CPUID-related assembler stuff
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

[GLOBAL cpuid_check]

cpuid_unsupported:
	mov eax, 0
	ret

cpuid_check:
	; Load flags to ECX and EAX
	pushfd
	pop ecx
	mov eax, ecx
	 
	; Invert the ID flag
	xor eax, 0x200000
	 
	; Load EAX to EFLAGS
	push eax
	popfd
	 
	; If CPUID is supported, the new ID flag will stay changed.
	 
	; Load Flags to EAX
	pushfd
	pop eax
	 
	; Compare before (ECX) and afterwards (EAX).
	; If both are the same, CPUID is _not_ supported.
	xor eax, ecx
	; Probably there's a nicer solution to do that. Feel free to add it.
	je cpuid_unsupported
	mov eax, 1
	ret
	
