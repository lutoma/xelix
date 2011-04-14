/* idt.c: Initialization of the IDT
 * Copyright © 2010 Christoph Sünderhauf
 * Copyright © 2011 Lukas Martini
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

#include "idt.h"

#include <interrupts/interface.h>
#include <lib/log.h>

// A struct describing an interrupt gate.
typedef struct
{
	uint16_t base_lo;	// The lower 16 bits of the address to jump to when this interrupt fires.
	uint16_t sel;		// Kernel segment selector.
	uint8_t  always0;	// This must always be zero.
	uint8_t  flags;	// More flags. See documentation.
	uint16_t base_hi;	// The upper 16 bits of the address to jump to.
} __attribute__((packed))
idtEntry_t;

// A struct describing a pointer to an array of interrupt handlers.
// This is in a format suitable for giving to 'lidt'.
typedef struct
{
	uint16_t limit;
	uint32_t base; // The address of the first element in our idt_entry_t array.
} __attribute__((packed))
idtPtr_t;

extern void interrupts_handler0(void);
extern void interrupts_handler1(void);
extern void interrupts_handler2(void);
extern void interrupts_handler3(void);
extern void interrupts_handler4(void);
extern void interrupts_handler5(void);
extern void interrupts_handler6(void);
extern void interrupts_handler7(void);
extern void interrupts_handler8(void);
extern void interrupts_handler9(void);
extern void interrupts_handler10(void);
extern void interrupts_handler11(void);
extern void interrupts_handler12(void);
extern void interrupts_handler13(void);
extern void interrupts_handler14(void);
extern void interrupts_handler15(void);
extern void interrupts_handler16(void);
extern void interrupts_handler17(void);
extern void interrupts_handler18(void);
extern void interrupts_handler19(void);
extern void interrupts_handler20(void);
extern void interrupts_handler21(void);
extern void interrupts_handler22(void);
extern void interrupts_handler23(void);
extern void interrupts_handler24(void);
extern void interrupts_handler25(void);
extern void interrupts_handler26(void);
extern void interrupts_handler27(void);
extern void interrupts_handler28(void);
extern void interrupts_handler29(void);
extern void interrupts_handler30(void);
extern void interrupts_handler31(void);
extern void interrupts_handler32(void);
extern void interrupts_handler33(void);
extern void interrupts_handler34(void);
extern void interrupts_handler35(void);
extern void interrupts_handler36(void);
extern void interrupts_handler37(void);
extern void interrupts_handler38(void);
extern void interrupts_handler39(void);
extern void interrupts_handler40(void);
extern void interrupts_handler41(void);
extern void interrupts_handler42(void);
extern void interrupts_handler43(void);
extern void interrupts_handler44(void);
extern void interrupts_handler45(void);
extern void interrupts_handler46(void);
extern void interrupts_handler47(void);

idtEntry_t idtEntries[256];
idtPtr_t idtPtr;

extern void idt_load(uint32_t);
static void setGate(uint8_t, uint32_t, uint16_t, uint8_t);

static void setGate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
	idtEntries[num].base_lo = base & 0xFFFF;
	idtEntries[num].base_hi = (base >> 16) & 0xFFFF;

	idtEntries[num].sel     = sel;
	idtEntries[num].always0 = 0;
	// We must uncomment the OR below when we get to using user-mode.
	// It sets the interrupt gate's privilege level to 3.
	idtEntries[num].flags   = flags /* | 0x60 */;
}

void idt_init()
{
	idtPtr.limit = sizeof(idtEntry_t) * 256 -1;
	idtPtr.base  = (uint32_t)&idtEntries;

	memset(&idtEntries, 0, sizeof(idtEntry_t)*256);

	// Initialize master PIC
	outb(0x20, 0x11); // The PICs initialization command
	outb(0x21, 0x20); // Intnum for IRQ 0
	outb(0x21, 0x04); // IRQ 2 = Slave
	outb(0x21, 0x01); // ICW 4
 
	// Initialize slave PIC
	outb(0xa0, 0x11); // The PICs initialization command
	outb(0xa1, 0x28); // Intnum for IRQ 8
	outb(0xa1, 0x02); // IRQ 2 = Slave
	outb(0xa1, 0x01); // ICW 4

	// Activate all IRQs (demask)
	outb(0x20, 0x0);
	outb(0xa0, 0x0);

	
	log("idt: Initialized PICs / IRQs.\n");

	// ISR
	setGate(0, (uint32_t)interrupts_handler0, 0x08, 0x8E);
	setGate(1, (uint32_t)interrupts_handler1, 0x08, 0x8E);
	setGate(2, (uint32_t)interrupts_handler2, 0x08, 0x8E);
	setGate(3, (uint32_t)interrupts_handler3, 0x08, 0x8E);
	setGate(4, (uint32_t)interrupts_handler4, 0x08, 0x8E);
	setGate(5, (uint32_t)interrupts_handler5, 0x08, 0x8E);
	setGate(6, (uint32_t)interrupts_handler6, 0x08, 0x8E);
	setGate(7, (uint32_t)interrupts_handler7, 0x08, 0x8E);
	setGate(8, (uint32_t)interrupts_handler8, 0x08, 0x8E);
	setGate(9, (uint32_t)interrupts_handler9, 0x08, 0x8E);
	setGate(10, (uint32_t)interrupts_handler10, 0x08, 0x8E);
	setGate(11, (uint32_t)interrupts_handler11, 0x08, 0x8E);
	setGate(12, (uint32_t)interrupts_handler12, 0x08, 0x8E);
	setGate(13, (uint32_t)interrupts_handler13, 0x08, 0x8E);
	setGate(14, (uint32_t)interrupts_handler14, 0x08, 0x8E);
	setGate(15, (uint32_t)interrupts_handler15, 0x08, 0x8E);
	setGate(16, (uint32_t)interrupts_handler16, 0x08, 0x8E);
	setGate(17, (uint32_t)interrupts_handler17, 0x08, 0x8E);
	setGate(18, (uint32_t)interrupts_handler18, 0x08, 0x8E);
	setGate(19, (uint32_t)interrupts_handler19, 0x08, 0x8E);
	setGate(20, (uint32_t)interrupts_handler20, 0x08, 0x8E);
	setGate(21, (uint32_t)interrupts_handler21, 0x08, 0x8E);
	setGate(22, (uint32_t)interrupts_handler22, 0x08, 0x8E);
	setGate(23, (uint32_t)interrupts_handler23, 0x08, 0x8E);
	setGate(24, (uint32_t)interrupts_handler24, 0x08, 0x8E);
	setGate(25, (uint32_t)interrupts_handler25, 0x08, 0x8E);
	setGate(26, (uint32_t)interrupts_handler26, 0x08, 0x8E);
	setGate(27, (uint32_t)interrupts_handler27, 0x08, 0x8E);
	setGate(28, (uint32_t)interrupts_handler28, 0x08, 0x8E);
	setGate(29, (uint32_t)interrupts_handler29, 0x08, 0x8E);
	setGate(30, (uint32_t)interrupts_handler30, 0x08, 0x8E);
	setGate(31, (uint32_t)interrupts_handler31, 0x08, 0x8E);
	setGate(32, (uint32_t)interrupts_handler32, 0x08, 0x8E);
	setGate(33, (uint32_t)interrupts_handler33, 0x08, 0x8E);
	setGate(34, (uint32_t)interrupts_handler34, 0x08, 0x8E);
	setGate(35, (uint32_t)interrupts_handler35, 0x08, 0x8E);
	setGate(36, (uint32_t)interrupts_handler36, 0x08, 0x8E);
	setGate(37, (uint32_t)interrupts_handler37, 0x08, 0x8E);
	setGate(38, (uint32_t)interrupts_handler38, 0x08, 0x8E);
	setGate(39, (uint32_t)interrupts_handler39, 0x08, 0x8E);
	setGate(40, (uint32_t)interrupts_handler40, 0x08, 0x8E);
	setGate(41, (uint32_t)interrupts_handler41, 0x08, 0x8E);
	setGate(42, (uint32_t)interrupts_handler42, 0x08, 0x8E);
	setGate(43, (uint32_t)interrupts_handler43, 0x08, 0x8E);
	setGate(44, (uint32_t)interrupts_handler44, 0x08, 0x8E);
	setGate(45, (uint32_t)interrupts_handler45, 0x08, 0x8E);
	setGate(46, (uint32_t)interrupts_handler46, 0x08, 0x8E);
	setGate(47, (uint32_t)interrupts_handler47, 0x08, 0x8E);
	
	idt_load((uint32_t)&idtPtr);
	interrupts_enable();
}
