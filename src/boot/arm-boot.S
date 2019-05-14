/* arm-boot.S: ARM initialization
 * Copyright © 2019 Lukas Martini
 *
 * This file is part of Xelix.
 *
 * Xelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

.section ".bss"
.global stack_begin
.global stack_end
.global int_stack_begin
.global int_stack_end
.balign 8
.lcomm stack_begin, 1024 * 16
stack_end:
.lcomm int_stack_begin, 1024 * 16
int_stack_end:

.section ".text.boot"
.global _start
.global arm_halt

_start:
	// Shut off extra cores
	mrc p15, 0, r5, c0, c0, 5
	and r5, r5, #3
	cmp r5, #0
	bne arm_halt

	// Switch to SVC if the bootloader has left us in HYP mode
	mrs	r9, cpsr
	// test for HYP mode
	eor	r9, r9, #0x1A
	tst	r9, #0x1F
	// clear mode bits
	bic	r9, r9, #0x1F
	// mask IRQ/FIQ bits and set SVC mode
	orr	r9, r9, #0xC0 | 0x13
	// branch if not HYP mode
	bne	1f
	// mask Abort bit
	orr	r9, r9, #0x100
	adr	lr, 2f
	msr	spsr_cxsf, r9
	msr ELR_hyp, lr
	eret
1:	msr	cpsr_c, r9
2:

	// Clear BSS
	ldr r0, =__bss_start
	mov r1, #0
	ldr r2, =__bss_len
	bl memset

	// Stack setup for supervisor / irq mode
	ldr r5, =stack_end
	mov sp, r5

	mrs r0, cpsr
	bic r0, $0b11111111
	orr r0, $0b11010010
	msr cpsr, r0
	ldr r5, =int_stack_end
	mov sp, r5
	bic r0, $0b11111111
	orr r0, $0b11010011
	msr cpsr, r0

	bl xelix_main

arm_halt:
	wfe
	b arm_halt
