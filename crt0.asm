SECTION .text
GLOBAL __crt0_entry
EXTERN main

__crt0_entry:
	call main
	
	; Exit
	mov eax, 1
	int 80h

	; Loop in case we haven't rescheduled yet
j:	hlt
	nop
	nop
	jmp j
