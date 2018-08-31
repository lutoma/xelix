; The most basic xelix executable possible.

GLOBAL _start
_start:
	; Call the test syscall
	mov eax, 9
	int 80h

	; Call the exit syscall
	mov eax, 1
	int 80h

.il:
	jmp .il
