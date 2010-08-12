

; in scheduler.c
[EXTERN schedule]

global switchcontext

; this is called when the IRQ0 timer interrupt is fired.
; This is called directly, without any modification to the stack.
switchcontext:
	; push registers to stack (eflags, cs and eip (of the last instruction of the process before this irq) have already been pushed automatically by the interrupt) 
	pusha ; push general registers
	push ds ; push data segment registers
	push es
	push fs
	push gs
	
	; we have to switch to kernel data selectors because they are still in user mode (once we implement user mode ;) )
	mov eax, 0x10
	mov ds, eax
	mov es, eax
	mov fs, eax
	mov gs, eax
	
	; give esp (pointing to the top (in x86 bottom in terms of hardware) of the stack so far) as a paramter (push it to stack according to cdecl calling convention)
	mov eax, esp
	push eax
	;call schedule ; this causes an error
	
	; extract the return value of schedule() (according to cdecl calling convention it is in eax) and thus set the stack pointer for the new process
	mov esp, eax
	
	; ack IRQ
	mov al, 0x20
	out 0x20, al
	
	; retrieve registers of new process
	pop gs
	pop fs
	pop es
	pop ds
	popa
	
	
	sti
	; interrupt return, because of automatically pushed stuff in the new process' stack continues excecution at the point the new process was last pre-empted.
	iret

