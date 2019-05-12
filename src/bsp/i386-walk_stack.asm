; Walks backwards through the call stack and builds a list of return addresses.
; Args:
;  * Array of 32-bit addresses.
;  * Maximum number of elements in array.
; Return value: The number of addresses stored in the array.
; Calling convention: cdecl
[global walk_stack]
walk_stack:
	; Create stack frame & save caller's EDI and EBX.
	push ebp
	mov  ebp,       esp
	mov  [ebp - 4], edi
	mov  [ebp - 8], ebx
	; Set up local registers.
	xor  eax,       eax         ; EAX = return value (number of stack frames found).
	mov  ebx,       [esp +  0]  ; EBX = old EBP.
	mov  edi,       [esp +  8]  ; Destination array pointer in EDI.
	mov  ecx,       [esp + 12]  ; Maximum array size in ECX.
.walk:
	; Walk backwards through EBP linked list, storing return addresses in EDI array.
	mov  edx,       [ebx +  4]  ; EDX = previous stack frame's IP.
	test edx,       edx
	jz   .done                  ; Leave if it is 0.
	mov  ebx,       [ebx +  0]  ; EBX = previous stack frame's BP.
	mov  [edi],     edx         ; Copy IP.
	add  edi,       4
	inc  eax
	loop .walk
.done:
	; Restore caller's EDI and EBX, leave stack frame & return EAX.
	mov  edi,       [ebp - 4]
	mov  ebx,       [ebp - 8]
	mov  esp,       ebp
	pop  ebp
	ret
