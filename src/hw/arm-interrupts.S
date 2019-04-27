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
.extern stack_end
.extern _start

// Interrupt/exception vector table at 0x0
.section ".init"
.word stack_end
b _start			// Reset
b arm_handle_nmi	// NMI
b arm_handle_hflt	// Hard fault
b arm_handle_mmflt	// Memory mgmt. fault
b arm_handle_busflt	// Bus fault
b arm_handle_usflt	// Usage fault
b .					// Reserved
b .					// Reserved
b .					// Reserved
b .					// Reserved
b arm_handle_svcall	// SVCall
b arm_handle_debug	// Debug
b .					// Reserved
b arm_handle_pendsv	// PendSV
b arm_handle_tick	// Systick
b arm_handle_irq0	// IRQ0
b arm_handle_irq1	// IRQ1
b arm_handle_irq2	// IRQ2

.section ".text"

arm_handle_nmi:
arm_handle_hflt:
arm_handle_mmflt:
arm_handle_busflt:
arm_handle_usflt:
arm_handle_svcall:
arm_handle_debug:
arm_handle_pendsv:
arm_handle_tick:
arm_handle_irq0:
arm_handle_irq1:
arm_handle_irq2:
    //srsdb sp!, #0b10011
    cpsie aif, #0b10011
	b cpu_fault_handler
	b .

arm_handle_irq:
	sub     r14, #4
	// Store non-banked registers
	push    {r0 - r12, r14}

	// Store supervisor R13/R14/SPSR & CPSR
	msr     cpsr_ctl, #0xD3
	mrs     r1, spsr
	mov     r2, r13
	mov     r3, r14
	msr     cpsr_ctl, #0xD2
	mrs     r0, spsr
	push    {r0 - r3}

	// Store user R13/R14
	stmdb   r13, {r13, r14}^
	sub     r13, #8

	// Call C handler
	mov		r0, #42
	mov     r1, r13
	bl      interrupts_callback
	mov     r13, r0

	// Reload registers from above
	ldmia   r13, {r13, r14}^
	add     r13, #8

	pop     {r0 - r3}
	msr     spsr_all, r0
	msr     cpsr_ctl, #0xD3
	msr     spsr_all, r1
	mov     r13, r2
	mov     r14, r3
	msr     cpsr_all, #0xD2

	pop     {r0 - r12, r14}
	movs    r15, r14
