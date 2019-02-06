; interrupts.asm: Hardware part of interrupt handling
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

[EXTERN cpu_fault_handler]
[EXTERN interrupts_callback]
[EXTERN paging_kernel_cr3]

%define PIT_MASTER	0x20
%define PIT_SLAVE	0xA0
%define PIT_CONFIRM	0x20
%define IRQ7		39
%define IRQ15		47

%macro INTERRUPT 1
	[GLOBAL interrupts_handler%1]
	interrupts_handler%1:
		push esp
		pusha
		mov ebx, %1
		jmp interrupts_common_handler
%endmacro

%macro INTERRUPT_FAULT 1
	[GLOBAL interrupts_handler%1]
	interrupts_handler%1:
		cli
		mov ebx, %1
		jmp interrupts_fault_handler
%endmacro

%assign i 0
%rep 256
	%if i < 32
		INTERRUPT_FAULT i
	%else
		INTERRUPT i
	%endif
	%assign i i+1
%endrep

; Handles interrupt hardware handling. Expects interrupt number in ebx. Returns
; 1 in eax if the interrupt was spurious, 0 otherwise.
handle_eoi:
	; Is this a spurious interrupt on the master PIC? If yes, return
	cmp ebx, IRQ7
	je .spurious

	; Do we have to send an EOI (End of interrupt)?
	cmp ebx, 31
	jle .return

	; Send EOI to master PIC
	mov al, PIT_CONFIRM
	out PIT_MASTER, al

	; Is this a spurious interrupt on the secondary PIC? If yes, return
	; (We do this here so the master PIC still receives an EOI as it can't
	; know about the spuriousness of this interrupt).
	cmp ebx, IRQ15
	je .spurious

	; Check if we have to send an EOI to the secondary PIC
	cmp ebx, 39
	jle .return

	; Send EOI to secondary PIC
	mov al, PIT_CONFIRM
	out PIT_SLAVE, al
.return:
	mov eax, 0
	ret
.spurious:
	mov eax, 1
	ret


interrupts_fault_handler:
	; Disable paging
	mov eax, cr0
	and eax, ~(1 << 31)
	mov cr0, eax

	call handle_eoi

	mov ecx, ebx
	mov edx, cr2
	call cpu_fault_handler
	mov esp, eax

	; reload paging context
	pop eax
	cmp eax, [paging_kernel_cr3]
	je isf_return

	; Re-enable paging
	mov cr3, eax
	mov eax, cr0
	or eax, (1 << 31)
	mov cr0, eax

	jmp isf_return


; This is our common Interrupt handler. It saves the processor state, sets up
; for kernel mode segments, handles spurious interrupts, calls the C-level
; handler, and finally restores the stack frame.

interrupts_common_handler:
	; We have to push all the stuff in isf_t (hw/interrupts.h) which
	; interrupts_callback takes in reversed order.
	;
	; The cpu automatically pushes eflags, cs, and eip. Our macros above push
	; the error code (if any, otherwise 0), the interrupt's number, and cr2
	; (for page faults). The rest is up to us.

	; push ds & cr3
	xor eax, eax
	mov ax, ds
	push eax

	mov eax, cr3
	push eax

	; load the kernel data segment descriptor
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; ebx still contains the interrupt number from the handlers above
	call handle_eoi
	test eax, eax
	jnz .return

	mov ecx, [paging_kernel_cr3]
	jecxz .no_paging

	mov edx, cr3
	cmp ecx, edx
	je .no_paging
	mov cr3, ecx
.no_paging:

	; fastcall
	mov ecx, ebx
	mov edx, esp
 	call interrupts_callback

	; interrupts_callback returns an interrupt stack frame
	mov esp, eax

.return:
	; reload paging context
	pop eax
	cmp eax, [paging_kernel_cr3]
	je isf_return
	mov cr3, eax

isf_return:
	; reload segment descriptors
	pop eax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	popa
	pop esp
	iret
