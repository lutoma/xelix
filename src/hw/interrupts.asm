; interrupts.asm: Hardware part of interrupt handling
; Copyright © 2010-2016 Lukas Martini

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

%define PIT_MASTER	0x20
%define PIT_SLAVE	0xA0
%define PIT_CONFIRM	0x20
%define IRQ7		39
%define IRQ15		47

%macro INTERRUPT 1
	[GLOBAL interrupts_handler%1]
	interrupts_handler%1:
		push esp

		; Dummy value for error code
		push 0
		push %1
		; Dummy value for cr2
		push 0
		jmp interrupts_common_handler
%endmacro

%macro INTERRUPT_ERRCODE 1
	[GLOBAL interrupts_handler%1]
	interrupts_handler%1:
		push esp

		push %1
		; Dummy value for cr2
		push 0
		jmp interrupts_common_handler
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

; Special handler for page faults that pushes cr2
[GLOBAL interrupts_handler14]
interrupts_handler14:
	push esp
	push 14

	; The cr2 register contains the accessed address in case of page faults.
	mov eax, cr2
	push eax
	jmp interrupts_common_handler

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

interrupts_common_handler:
	cli
	; We have to push all the stuff in the cpu_state_t (hw/cpu.h) which
	; interrupts_callback takes in reversed order.
	;
	; The cpu automatically pushes eflags, cs, and eip. Our macros above push
	; the error code (if any, otherwise 0), the interrupt's number, and cr2
	; (for page faults). The rest is up to us.

	pusha

	; push ds
	xor eax, eax
	mov ax, ds
	push eax

	; load the kernel data segment descriptor
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; Calculate offset to the position of the interrupt number on the
	; stack and mov it to eax. (10 = Ten 32 bit registers pushed since
	; the beginning of this function).
	mov ebx, [esp + (4 * 10)]

	; Is this a spurious interrupt on the master PIC? If yes, return
	cmp ebx, IRQ7
	je .return

	; Do we have to send an EOI (End of interrupt)?
	cmp ebx, 31
	jle .no_eoi

	; Send EOI to master PIC
	mov dx, PIT_MASTER
	mov ax, PIT_CONFIRM
	out dx, ax

	; Is this a spurious interrupt on the secondary PIC? If yes, return
	; (We do this here so the master PIC still get's an EOI as it can't
	; know about the spuriousness of this interrupt).
	cmp ebx, IRQ15
	je .return

	; Check if we have to send an EOI to the secondary PIC
	cmp ebx, 39
	jle .no_eoi

	; Send EOI to secondary PIC
	mov dx, PIT_SLAVE
	mov ax, PIT_CONFIRM
	out dx, ax

.no_eoi:
	; fastcall
	mov ecx, esp
 	call interrupts_callback

	; Use cpu_state_t as stack
	mov esp, eax

.return:
	; reload the original data segment descriptor
	pop ebx
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	; Reload the original values of the GP registers
	popa

	; Restore original stack
	add esp, 3*4
	pop esp

	; Now, quit interrupthandler. This automatically pops cs, eip,
	; eflags.
	iret
