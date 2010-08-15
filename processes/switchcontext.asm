

; in scheduler.c
[EXTERN schedule]

global switchcontext

; this is called when the IRQ0 timer interrupt is fired.
; This is called directly, without any modification to the stack.
switchcontext:
	; push registers to stack (eflags, cs and eip (of the last instruction of the process before this irq) 
	;have already been pushed automatically by the interrupt) 
	
	push 0 ; err code
	push 32 ; int no
	
	pusha ; push general registers
	push ds ; push data segment register
	
	; we have to switch to kernel data selectors because they are still in user mode (once we implement user mode ;) )
	mov eax, 0x10
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	
	call schedule
	; return value of schedule() is now in eax (cdecl calling convention)
	
	; if the return value is 0, it means multiprocessing is not enabled yet -> don't change the stack
	cmp eax, 0
	je nomultiprocessing
	
	; set the stack pointer for the new process
	mov esp, eax
	
quitinterrupt:
	
	; ack IRQ
	mov al, 0x20
	out 0x20, al
	; retrieve registers of new process (or old process if nomultiprocessing as stack wasn't changed)
	pop eax
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	popa
	
	add esp, 8 ; pretend to pop the int_no and err_code
	
	sti
	; interrupt return, because of automatically pushed stuff in the new process' stack continues excecution at the point the new process was last pre-empted.
	iret
	
nomultiprocessing:
	jmp quitinterrupt
