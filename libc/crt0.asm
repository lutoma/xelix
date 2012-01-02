SECTION .bss
GLOBAL errno
GLOBAL __environ

errno: resb 32
__environ: resb 32

SECTION .text
GLOBAL _start
EXTERN main

_start:
	; Push fake argc and argv
	xor eax, eax
	push eax
	push eax

	call main
	
	; Exit
	add esp, 4
	mov eax, 1
	int 80h

	; Loop in case we haven't rescheduled yet
j:	hlt
	nop
	nop
	jmp j
