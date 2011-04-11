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


[GLOBAL idt_flush]	 ; Allows the C code to call idt_flush().

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
		jmp commonStub
%endmacro

%macro ISR_ERRCODE 1
	[GLOBAL isr%1]
	isr%1:
		cli
		push byte %1
		jmp commonStub
%endmacro

; This macro creates a stub for an IRQ - the first parameter is
; the IRQ number, the second is the ISR number it is remapped to.
%macro IRQ 2
	[GLOBAL irq%1]
	irq%1:
		cli
		push byte 0
		push byte %2
		jmp commonStub
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

IRQ	0,	 32
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
	; number. The rest is up to us. Luckily, there's pusha to push 'em
	; all.
	pusha			; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

	; Lower 16-bits of eax = ds. I'm not quite sure why we save that
	; one, as this conversion could also be done in C code, but all the
	; other kernels out there also do it that way, so i'll stick with
	; that.
	mov ax, ds
	push eax

	; ??? („What have the humans done? EXPLAIN! EXPLAIN! EXPLAIN!“)
	mov ax, 0x10	; load the kernel data segment descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
 	push esp
 	call interrupts_firstCallBack
	mov esp, eax
	
	pop ebx			; reload the original data segment descriptor
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	popa			; Pops edi,esi,ebp...
	add esp, 8		; Cleans up the pushed error code and pushed ISR number
	sti
	iret			; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP
