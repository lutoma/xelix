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

.extern interrupts_callback
.extern _start

// Interrupt/exception vector table at 0x0
.section ".init"
b arm_handle_fault	// Reset
b arm_handle_fault	// Undefined instruction
b arm_handle_irq	// Software interrupt
b arm_handle_fault	// Abort (prefetch)
b arm_handle_fault	// Abort (data)
b .					// Reserved
b arm_handle_irq	// IRQ0
b arm_handle_irq	// IRQ1

.section ".text"
arm_handle_fault:
    //srsdb sp!, #0b10011
    cpsie aif, #0b10011
	b cpu_fault_handler
	b .

arm_handle_irq:
	//sub     lr, #4 // ???

	// Store non-banked registers
	push    {r0 - r12, lr}

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
	mov		r0, #42
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
