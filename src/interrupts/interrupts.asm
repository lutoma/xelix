; interrupts.asm: Hardware part of interrupt handling
; Copyright © 2010 Christoph Sünderhauf

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


[GLOBAL idt_flush]	 ; Allows the C code to call idt_flush().
[GLOBAL irq0]

idt_flush:
	mov eax, [esp+4]  ; Get the pointer to the IDT, passed as a parameter. 
	lidt [eax]		  ; Load the IDT pointer.
	ret

%macro ISR_NOERRCODE 1  ; define a macro, taking one parameter
	[GLOBAL isr%1]		  ; %1 accesses the first parameter.
	isr%1:
		cli
		push byte 0
		push byte %1
		jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
	[GLOBAL isr%1]
	isr%1:
		cli
		push byte %1
		jmp isr_common_stub
%endmacro

; This macro creates a stub for an IRQ - the first parameter is
; the IRQ number, the second is the ISR number it is remapped to.
%macro IRQ 2
	global irq%1
	irq%1:
		cli
		push byte 0
		push byte %2
		jmp irq_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE 8
ISR_NOERRCODE 9
ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

irq0:
	cli
	call scheduler_select
	push byte 0
	push byte 32
	jmp irq_common_stub

[EXTERN scheduler_select]

IRQ	1,	 33
IRQ	2,	 34
IRQ	3,	 35
IRQ	4,	 36
IRQ	5,	 37
IRQ	6,	 38
IRQ	7,	 39
IRQ	8,	 40
IRQ	9,	 41
IRQ  10,	 42
IRQ  11,	 43
IRQ  12,	 44
IRQ  13,	 45
IRQ  14,	 46
IRQ  15,	 47


; syscalls
global isr81
extern syscalls_handler_stub
isr81:
	jmp syscalls_handler_stub

; In isr.c
[EXTERN isr_handler]

; This is our common ISR stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame.
isr_common_stub:
	pusha			; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

	mov ax, ds		; Lower 16-bits of eax = ds.
	push eax		; save the data segment descriptor

	mov ax, 0x10	; load the kernel data segment descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	call isr_handler

	pop eax			; reload the original data segment descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	popa			; Pops edi,esi,ebp...
	add esp, 8		; Cleans up the pushed error code and pushed ISR number
	sti
	iret			; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP

; In isr.c
[EXTERN irq_handler]

; This is our common IRQ stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame. 
irq_common_stub:
	pusha			; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

	mov ax, ds		; Lower 16-bits of eax = ds.
	push eax		; save the data segment descriptor

	mov ax, 0x10	; load the kernel data segment descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	call irq_handler

	pop ebx			; reload the original data segment descriptor
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	popa			; Pops edi,esi,ebp...
	add esp, 8		; Cleans up the pushed error code and pushed ISR number
	sti
	iret			; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP
