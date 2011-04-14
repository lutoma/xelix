; interrupts.asm: Hardware part of interrupt handling
; Copyright © 2010 Christoph Sünderhauf
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
; along with Xelix.  If not, see <http://www.gnu.org/licenses/>.

%macro INTERRUPT 1
	[GLOBAL interrupts_handler%1]
	interrupts_handler%1:
		cli
		push 0
		push %1
		jmp commonStub
%endmacro

%macro INTERRUPT_ERRCODE 1
	[GLOBAL interrupts_handler%1]
	interrupts_handler%1:
		cli
		push %1
		jmp commonStub
%endmacro

INTERRUPT 0
INTERRUPT 1
INTERRUPT 2
INTERRUPT 3
INTERRUPT 4
INTERRUPT 5
INTERRUPT 6
INTERRUPT 7
INTERRUPT_ERRCODE 8
INTERRUPT 9
INTERRUPT_ERRCODE 10
INTERRUPT_ERRCODE 11
INTERRUPT_ERRCODE 12
INTERRUPT_ERRCODE 13
INTERRUPT_ERRCODE 14
INTERRUPT 15
INTERRUPT 16
INTERRUPT 17
INTERRUPT 18
INTERRUPT 19
INTERRUPT 20
INTERRUPT 21
INTERRUPT 22
INTERRUPT 23
INTERRUPT 24
INTERRUPT 25
INTERRUPT 26
INTERRUPT 27
INTERRUPT 28
INTERRUPT 29
INTERRUPT 30
INTERRUPT 31
INTERRUPT 32
INTERRUPT 33
INTERRUPT 34
INTERRUPT 35
INTERRUPT 36
INTERRUPT 37
INTERRUPT 38
INTERRUPT 39
INTERRUPT 40
INTERRUPT 41
INTERRUPT 42
INTERRUPT 43
INTERRUPT 44
INTERRUPT 45
INTERRUPT 46
INTERRUPT 47
INTERRUPT 48
INTERRUPT 49
INTERRUPT 50
INTERRUPT 51
INTERRUPT 52
INTERRUPT 53
INTERRUPT 54
INTERRUPT 55
INTERRUPT 56
INTERRUPT 57
INTERRUPT 58
INTERRUPT 59
INTERRUPT 60
INTERRUPT 61
INTERRUPT 62
INTERRUPT 63
INTERRUPT 64
INTERRUPT 65
INTERRUPT 66
INTERRUPT 67
INTERRUPT 68
INTERRUPT 69
INTERRUPT 70
INTERRUPT 71
INTERRUPT 72
INTERRUPT 73
INTERRUPT 74
INTERRUPT 75
INTERRUPT 76
INTERRUPT 77
INTERRUPT 78
INTERRUPT 79
INTERRUPT 80
INTERRUPT 81
INTERRUPT 82
INTERRUPT 83
INTERRUPT 84
INTERRUPT 85
INTERRUPT 86
INTERRUPT 87
INTERRUPT 88
INTERRUPT 89
INTERRUPT 90
INTERRUPT 91
INTERRUPT 92
INTERRUPT 93
INTERRUPT 94
INTERRUPT 95
INTERRUPT 96
INTERRUPT 97
INTERRUPT 98
INTERRUPT 99
INTERRUPT 100
INTERRUPT 101
INTERRUPT 102
INTERRUPT 103
INTERRUPT 104
INTERRUPT 105
INTERRUPT 106
INTERRUPT 107
INTERRUPT 108
INTERRUPT 109
INTERRUPT 110
INTERRUPT 111
INTERRUPT 112
INTERRUPT 113
INTERRUPT 114
INTERRUPT 115
INTERRUPT 116
INTERRUPT 117
INTERRUPT 118
INTERRUPT 119
INTERRUPT 120
INTERRUPT 121
INTERRUPT 122
INTERRUPT 123
INTERRUPT 124
INTERRUPT 125
INTERRUPT 126
INTERRUPT 127
INTERRUPT 128
INTERRUPT 129
INTERRUPT 130
INTERRUPT 131
INTERRUPT 132
INTERRUPT 133
INTERRUPT 134
INTERRUPT 135
INTERRUPT 136
INTERRUPT 137
INTERRUPT 138
INTERRUPT 139
INTERRUPT 140
INTERRUPT 141
INTERRUPT 142
INTERRUPT 143
INTERRUPT 144
INTERRUPT 145
INTERRUPT 146
INTERRUPT 147
INTERRUPT 148
INTERRUPT 149
INTERRUPT 150
INTERRUPT 151
INTERRUPT 152
INTERRUPT 153
INTERRUPT 154
INTERRUPT 155
INTERRUPT 156
INTERRUPT 157
INTERRUPT 158
INTERRUPT 159
INTERRUPT 160
INTERRUPT 161
INTERRUPT 162
INTERRUPT 163
INTERRUPT 164
INTERRUPT 165
INTERRUPT 166
INTERRUPT 167
INTERRUPT 168
INTERRUPT 169
INTERRUPT 170
INTERRUPT 171
INTERRUPT 172
INTERRUPT 173
INTERRUPT 174
INTERRUPT 175
INTERRUPT 176
INTERRUPT 177
INTERRUPT 178
INTERRUPT 179
INTERRUPT 180
INTERRUPT 181
INTERRUPT 182
INTERRUPT 183
INTERRUPT 184
INTERRUPT 185
INTERRUPT 186
INTERRUPT 187
INTERRUPT 188
INTERRUPT 189
INTERRUPT 190
INTERRUPT 191
INTERRUPT 192
INTERRUPT 193
INTERRUPT 194
INTERRUPT 195
INTERRUPT 196
INTERRUPT 197
INTERRUPT 198
INTERRUPT 199
INTERRUPT 200
INTERRUPT 201
INTERRUPT 202
INTERRUPT 203
INTERRUPT 204
INTERRUPT 205
INTERRUPT 206
INTERRUPT 207
INTERRUPT 208
INTERRUPT 209
INTERRUPT 210
INTERRUPT 211
INTERRUPT 212
INTERRUPT 213
INTERRUPT 214
INTERRUPT 215
INTERRUPT 216
INTERRUPT 217
INTERRUPT 218
INTERRUPT 219
INTERRUPT 220
INTERRUPT 221
INTERRUPT 222
INTERRUPT 223
INTERRUPT 224
INTERRUPT 225
INTERRUPT 226
INTERRUPT 227
INTERRUPT 228
INTERRUPT 229
INTERRUPT 230
INTERRUPT 231
INTERRUPT 232
INTERRUPT 233
INTERRUPT 234
INTERRUPT 235
INTERRUPT 236
INTERRUPT 237
INTERRUPT 238
INTERRUPT 239
INTERRUPT 240
INTERRUPT 241
INTERRUPT 242
INTERRUPT 243
INTERRUPT 244
INTERRUPT 245
INTERRUPT 246
INTERRUPT 247
INTERRUPT 248
INTERRUPT 249
INTERRUPT 250

; In interrupts.c
[EXTERN interrupts_firstCallBack]

; This is our common Interrupt stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level handler,
; and finally restores the stack frame.

; As this is kinda complicated, i'll document every step here. (Mostly
; for forgetful me ;)) -- Lukas
commonStub:
	; We have to push all the stuff in the cpu_state_t which
	; interrupts_callback takes in reversed order
	; (It's defined in hw/cpu.h). The cpu automatically pushes cs, eip,
	; eflags, ss and esp. Our macros above push one byte containing the
	; error code (if any) and another one containing the interrupt's
	; number. The rest is up to us. We intentionally don't use pusha
	; (no need for esp).
	push eax
	push ecx
	push edx
	push ebx
	push ebp
	push esi
	push edi
	
	; push ds
	mov eax, 0
	mov ax, ds
	push eax

	; load the kernel data segment descriptor
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	; Push argument to ..
 	push esp
 	; Call C level interrupt handler
 	call interrupts_firstCallBack
	; Take esp from stack
	add esp, 4
	
	; Apply new stack
	mov esp, eax
	
	; reload the original data segment descriptor
	pop ebx
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	; Reload all the registers.
	pop edi
	pop esi
	pop ebp
	pop ebx
	pop edx
	pop ecx
	pop eax

	; Cleans up the pushed error code and pushed ISR number
	add esp, 8

	; Reenable interrupts
	sti
	
	; Now, quit interrupthandler. This automatically pops cs, eip,
	; eflags, css and esp.
	iret
