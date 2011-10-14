; interrupts.asm: Hardware part of interrupt handling
; Copyright © 2010 Christoph Sünderhauf
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
; along with Xelix.  If not, see <http://www.gnu.org/licenses/>.

%macro INTERRUPT 1
	[GLOBAL interrupts_handler%1]
	interrupts_handler%1:
		push 0
		push %1
		jmp commonStub
%endmacro

%macro INTERRUPT_ERRCODE 1
	[GLOBAL interrupts_handler%1]
	interrupts_handler%1:
		push %1
		jmp commonStub
%endmacro


INTERRUPT 0
INTERRUPT 1
INTERRUPT 2
INTERRUPT 3
INTERRUPT 4
INTERRUPT 5
INTERRUPT 6
INTERRUPT 7
INTERRUPT_ERRCODE 8
INTERRUPT 9
INTERRUPT_ERRCODE 10
INTERRUPT_ERRCODE 11
INTERRUPT_ERRCODE 12
INTERRUPT_ERRCODE 13
INTERRUPT_ERRCODE 14

; Assign the rest using a preprocessor loop
%assign i 15
%rep 241
	INTERRUPT i
	%assign i i+1
%endrep

; In interrupts.c
[EXTERN interrupts_firstCallBack]

; This is our common Interrupt stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level handler,
; and finally restores the stack frame.

; As this is kinda complicated, i'll document every step here. (Mostly
; for forgetful me ;)) -- Lukas
commonStub:
	; We have to push all the stuff in the cpu_state_t which
	; interrupts_callback takes in reversed order
	; (It's defined in hw/cpu.h). The cpu automatically pushes cs, eip,
	; eflags, ss and esp. Our macros above push one byte containing the
	; error code (if any) and another one containing the interrupt's
	; number. The rest is up to us. We intentionally don't use pusha
	; (no need for esp).
	push eax
	push ecx
	push edx
	push ebx
	push ebp
	push esi
	push edi
	
	; push ds
	xor eax, eax ; Move 0 to eax by xor-ing it with itself
	mov ax, ds
	push eax

	; load the kernel data segment descriptor
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	; Push argument to ..
 	push esp

 	; Call C level interrupt handler
 	call interrupts_firstCallBack

	; Take esp from stack. This is uncommented as we directly apply a new stack
	;add esp, 4
	
	; Apply new stack
	mov esp, eax
	
	; reload the original data segment descriptor
	pop ebx
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	; Reload all the registers.
	pop edi
	pop esi
	pop ebp
	pop ebx
	pop edx
	pop ecx
	pop eax

	; Cleans up the pushed error code and pushed ISR number
	add esp, 8

	; Now, quit interrupthandler. This automatically pops cs, eip,
	; eflags, css and esp.
	iret
