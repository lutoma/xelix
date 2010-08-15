

extern syscallHandler

global syscalls_handler_stub

syscalls_handler_stub:
	
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
	
	;call syscallHandler ; call C function
	
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

extern p

call_syscall:
	push ebx ; save register, as we are going to modify it
	pusha
	;mov eax,[esp+8]
	;mov ebx,[esp+12]
	
	
	;int 81
	
	call p
	
	popa
	pop ebx
	
	ret
