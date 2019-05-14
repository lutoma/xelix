/* arm-interrupts.S: ARM interrupt handling
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

#define ARM_INT_TYPE_SVC 1
#define ARM_INT_TYPE_IRQ 2
#define ARM_INT_TYPE_FIQ 3

.extern interrupts_callback
.global arm_exception_vectors

// Interrupt/exception vector table. Copied to 0x0 on interrupt init
.section ".text"
.balign 32
arm_exception_vectors:
b arm_handle_reset	// Reset
b arm_handle_udi	// Undefined instruction
b arm_handle_svc	// Software interrupt
b arm_handle_abrt	// Abort (prefetch)
b arm_handle_abrtd	// Abort (data)
b .					// Reserved
b arm_handle_irq	// IRQ
b arm_handle_fiq	// FIQ

arm_handle_fault:
    // Disable exceptions, get IFSR & IFAR
    cpsie aif, #0b10011
    mrc p15, 0, r0, c5, c0, 1
    mrc p15, 0, r1, c6, c0, 2
	b cpu_fault_handler

.macro pre_handler, name, lroffset, typenum
arm_handle_\name:
	sub lr, #\lroffset
	push {r12, lr}
	mov r12, #\typenum
	b arm_common_handler
.endm

pre_handler reset, 0, 43
pre_handler udi, 4, 42
pre_handler svc, 0, ARM_INT_TYPE_SVC
pre_handler abrt, 4, 4
pre_handler abrtd, 8, 5
pre_handler irq, 4, ARM_INT_TYPE_IRQ
pre_handler fiq, 4, ARM_INT_TYPE_FIQ

arm_common_handler:
	// Store non-banked registers
	push    {r0 - r11}

	// Store supervisor sp/lr/SPSR & CPSR
/*	msr     cpsr_ctl, #0xD3
	mrs     r1, spsr
	mov     r2, sp
	mov     r3, lr
	msr     cpsr_ctl, #0xD2
	mrs     r0, spsr
	push    {r0 - r3}
*/
	// Store user sp/lr
	stmdb   sp, {sp, lr}^
	sub     sp, #8

	// Call C handler
	mov		r0, r12
	mov     r1, sp
	bl      interrupts_callback
	mov     sp, r0

	// Reload registers from above
	ldmia   sp, {sp, lr}^
	add     sp, #8

/*
	pop     {r0 - r3}
	msr     spsr_all, r0
	msr     cpsr_ctl, #0xD3
	msr     spsr_all, r1
	mov     sp, r2
	mov     lr, r3
	msr     cpsr_all, #0xD2
*/
	pop     {r0 - r12, lr}
	movs    pc, lr
