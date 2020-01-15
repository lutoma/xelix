; The most basic xelix executable possible.

section .data
	hello:	db 'Hello World.',10
	hellol:	equ $-hello

section .text
global _start
_start:
	; Call the write syscall
	mov eax, 3
	mov ebx, 1
	mov ecx, hello
	mov edx, hellol
	int 80h

	; Call the exit syscall
	mov eax, 1
	int 80h

.il:
	jmp .il
