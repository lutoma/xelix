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
.lcomm stack_begin, 1024 * 8
stack_end:
.global int_stack_begin
.global int_stack_end
.lcomm int_stack_begin, 1024 * 8
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

	// Setup the stack.
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

	// Clear out bss.
	ldr r4, =__bss_start
	ldr r9, =__bss_end
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0
	b 2f

1:
	// store multiple at r4.
	stmia r4!, {r5-r8}

	// If we are still below bss_end, loop.
2:
	cmp r4, r9
	blo 1b

	ldr r3, =xelix_main
	blx r3

arm_halt:
	wfe
	b arm_halt
