; syscalls.asm: Hardware part of the syscall handler
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

extern syscallHandler
global syscalls_handler_stub

syscalls_handler_stub:
	cli
	
	push 0 ; no error code
	push 81 ; int no.
	
	pusha
	push ds ; push data segment register
	
	; we have to switch to kernel data selectors because they are still in user mode (once we implement user mode ;) )
	mov eax, 0x10
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	
	call syscallHandler ; call C function
	
	; retrieve registers
	pop eax
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	popa
	
	add esp, 8 ; pretend to pop the int_no and err_code
	
	sti
	
	iret

; later this needs to be moved, because it is included by user space programs
; low level function to trigger a syscall needs the stack to be set up according to the cdecl calling convention with
; call_syscall(eax=syscall no., ebx=arg1)
global call_syscall
call_syscall:
	push ebx ; save register, as we are going to modify it
	; eax will be modified anyway as that is the return value
	
	mov eax,[esp+8]
	mov ebx,[esp+12]
	
	int 81
	pop ebx
	ret
