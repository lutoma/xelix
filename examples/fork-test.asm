; This file tests the fork syscall. More specifically, it prints values from
; registers and stack that were set before the fork, to make sure they're
; preserved after the fork

[global _start]
_start:
	mov edx, 0xcafe

	mov eax, 0xaffe
	push eax

	mov eax, 22
	int 80h

	mov ebx, eax
	pop ecx

	mov eax, 9
	int 80h

freeze:
	nop
	nop
	nop
	jmp freeze

	mov eax, 1
	int 80h




