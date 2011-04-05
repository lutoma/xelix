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
#include "interface.h"

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

// These extern directives let us access the addresses of our ASM ISR handlers.
extern void isr0 ();
extern void isr1 ();
extern void isr2 ();
extern void isr3 ();
extern void isr4 ();
extern void isr5 ();
extern void isr6 ();
extern void isr7 ();
extern void isr8 ();
extern void isr9 ();
extern void isr10 ();
extern void isr11 ();
extern void isr12 ();
extern void isr13 ();
extern void isr14 ();
extern void isr15 ();
extern void isr16 ();
extern void isr17 ();
extern void isr18 ();
extern void isr19 ();
extern void isr20 ();
extern void isr21 ();
extern void isr22 ();
extern void isr23 ();
extern void isr24 ();
extern void isr25 ();
extern void isr26 ();
extern void isr27 ();
extern void isr28 ();
extern void isr29 ();
extern void isr30 ();
extern void isr31();
extern void isr32();

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

idtEntry_t idtEntries[256];
idtPtr_t idtPtr;

extern void idt_flush(uint32_t);
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
	setGate( 0, (uint32_t)isr0  , 0x08, 0x8E);
	setGate( 1, (uint32_t)isr1  , 0x08, 0x8E);
	setGate( 2, (uint32_t)isr2  , 0x08, 0x8E);
	setGate( 3, (uint32_t)isr3  , 0x08, 0x8E);
	setGate( 4, (uint32_t)isr4  , 0x08, 0x8E);
	setGate( 5, (uint32_t)isr5  , 0x08, 0x8E);
	setGate( 6, (uint32_t)isr6  , 0x08, 0x8E);
	setGate( 7, (uint32_t)isr7  , 0x08, 0x8E);
	setGate( 8, (uint32_t)isr8  , 0x08, 0x8E);
	setGate( 9, (uint32_t)isr9  , 0x08, 0x8E);
	setGate(10, (uint32_t)isr10 , 0x08, 0x8E);
	setGate(11, (uint32_t)isr11 , 0x08, 0x8E);
	setGate(12, (uint32_t)isr12 , 0x08, 0x8E);
	setGate(13, (uint32_t)isr13 , 0x08, 0x8E);
	setGate(14, (uint32_t)isr14 , 0x08, 0x8E);
	setGate(15, (uint32_t)isr15 , 0x08, 0x8E);
	setGate(16, (uint32_t)isr16 , 0x08, 0x8E);
	setGate(17, (uint32_t)isr17 , 0x08, 0x8E);
	setGate(18, (uint32_t)isr18 , 0x08, 0x8E);
	setGate(19, (uint32_t)isr19 , 0x08, 0x8E);
	setGate(20, (uint32_t)isr20 , 0x08, 0x8E);
	setGate(21, (uint32_t)isr21 , 0x08, 0x8E);
	setGate(22, (uint32_t)isr22 , 0x08, 0x8E);
	setGate(23, (uint32_t)isr23 , 0x08, 0x8E);
	setGate(24, (uint32_t)isr24 , 0x08, 0x8E);
	setGate(25, (uint32_t)isr25 , 0x08, 0x8E);
	setGate(26, (uint32_t)isr26 , 0x08, 0x8E);
	setGate(27, (uint32_t)isr27 , 0x08, 0x8E);
	setGate(28, (uint32_t)isr28 , 0x08, 0x8E);
	setGate(29, (uint32_t)isr29 , 0x08, 0x8E);
	setGate(30, (uint32_t)isr30 , 0x08, 0x8E);
	setGate(31, (uint32_t)isr31 , 0x08, 0x8E);

	// IRQ
	setGate(32, (uint32_t)irq0  , 0x08, 0x8E);
	setGate(33, (uint32_t)irq1  , 0x08, 0x8E);
	setGate(34, (uint32_t)irq2  , 0x08, 0x8E);
	setGate(35, (uint32_t)irq3  , 0x08, 0x8E);
	setGate(36, (uint32_t)irq4  , 0x08, 0x8E);
	setGate(37, (uint32_t)irq5  , 0x08, 0x8E);
	setGate(38, (uint32_t)irq6  , 0x08, 0x8E);
	setGate(39, (uint32_t)irq7  , 0x08, 0x8E);
	setGate(40, (uint32_t)irq8  , 0x08, 0x8E);
	setGate(41, (uint32_t)irq9  , 0x08, 0x8E);
	setGate(42, (uint32_t)irq10 , 0x08, 0x8E);
	setGate(43, (uint32_t)irq11 , 0x08, 0x8E);
	setGate(44, (uint32_t)irq12 , 0x08, 0x8E);
	setGate(45, (uint32_t)irq13 , 0x08, 0x8E);
	setGate(46, (uint32_t)irq14 , 0x08, 0x8E);
	setGate(47, (uint32_t)irq15 , 0x08, 0x8E);
	
	idt_flush((uint32_t)&idtPtr);
	interrupts_enable(); // Enable interrupts. Usually a good idea if you want to use them...
}
