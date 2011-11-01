SECTION .text
GLOBAL _start
EXTERN main

_start:
	; Push fake argc and argv
	mov eax, 0
	push eax
	push eax

	call main
	
	; Exit
	mov eax, 1
	int 80h

	; Loop in case we haven't rescheduled yet
j:	hlt
	nop
	nop
	jmp j
