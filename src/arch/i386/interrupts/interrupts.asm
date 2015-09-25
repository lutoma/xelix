; interrupts.asm: Hardware part of interrupt handling
; Copyright © 2010-2015 Lukas Martini

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

%define EOI_MASTER	0x20
%define EOI_SLAVE	0xA0
%define EOI_PORT	0x20
%define IRQ7		39
%define IRQ15		47

%macro INTERRUPT 1
	[GLOBAL interrupts_handler%1]
	interrupts_handler%1:
		push 0
		push %1
		jmp common_handler
%endmacro

%macro INTERRUPT_ERRCODE 1
	[GLOBAL interrupts_handler%1]
	interrupts_handler%1:
		push %1
		jmp common_handler
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
[EXTERN interrupts_callback]

; This is our common Interrupt handler. It saves the processor state, sets up
; for kernel mode segments, handles spurious interrupts, calls the C-level
; handler, and finally restores the stack frame.

common_handler:
	; We have to push all the stuff in the cpu_state_t which
	; interrupts_callback takes in reversed order
	; (It's defined in hw/cpu.h). The cpu automatically pushes cs, eip,
	; eflags, ss and esp. Our macros above push one byte containing the
	; error code (if any) and another one containing the interrupt's
	; number. The rest is up to us. We intentionally don't use pusha
	; (no need for esp).

	pusha

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

	; Calculate offset to the position of the interrupt number on the
	; stack and mov it to eax. (8 = Eight 32 bit registers pushed since
	; the beginning of this function).
	mov eax, [esp + (4 * 9)]

	; Is this a spurious interrupt on the master PIC? If yes, return
	cmp eax, IRQ7
	je return

	; Do we have to send an EOI (End of interrupt)?
	cmp eax, 31
	jle no_eoi

	; Send EOI to master PIC
	mov dx, EOI_PORT
	mov ax, EOI_MASTER
	out dx, ax

	; Is this a spurious interrupt on the secondary PIC? If yes, return
	; (We do this here so the master PIC still get's an EOI as it can't
	; know about the spuriousness of this interrupt).
	cmp eax, IRQ15
	je return

	; Check if we have to send an EOI to the secondary PIC
	cmp eax, 39
	jle no_eoi

	; Send EOI to secondary PIC
	mov ax, EOI_SLAVE
	out dx, ax

no_eoi:
	; Push argument to ..
 	push esp

 	; Call C level interrupt handler
 	call interrupts_callback

return:
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

	; Reload the original values of the GP registers
	pop edi
	pop esi
	pop ebp
	add esp, 4 ; Skip ESP
	pop ebx
	pop edx
	pop ecx
	pop eax

	; Cleans up the pushed error code and pushed ISR number
	add esp, 8

	; Now, quit interrupthandler. This automatically pops cs, eip,
	; eflags, css and esp.
	iret
