SECTION .text
GLOBAL _start
EXTERN main

_start:
	call main
	
	; Exit
	mov eax, 1
	int 80h

	; Loop in case we haven't rescheduled yet
j:	hlt
	nop
	nop
	jmp j
