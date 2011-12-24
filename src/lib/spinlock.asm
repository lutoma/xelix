; spinlock.asm: Generic spinlocks
; Copyright © 2011 Lukas Martini

; This file is part of Xelix.
;
; Xelix is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; Xelix is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with Xelix. If not, see <http://www.gnu.org/licenses/>.

GLOBAL spinlock_get
GLOBAL spinlock_release

spinlock_get:
	mov edi, [esp + 4] ; Destination pointer
	mov ecx, [esp + 8] ; Number of retries

test:
	xor eax, eax
	mov al, 0 ; What we expect it to be
	mov dl, 1 ; What we want it to be

	lock cmpxchg byte [edi], dl
	jnz retry

	mov eax, ecx
	ret

fail:
	mov eax, -1
	ret

retry:
	; Check if we already have reached numretries
	dec ecx
	cmp ecx, 0
	jle fail

	; Scheduler yield while we wait
	int 31h
	jmp test

spinlock_release:
	mov edi, [esp + 4] ; Destination pointer
	lock mov byte [edi], 0
	ret
