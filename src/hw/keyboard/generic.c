/* generic.c: A generic keyboard driver. Should work for every PS2
 * keyboard and for most USB keyboards (Depends on BIOS / Legacy
 * support)
 * 
 * Copyright © 2010 Christoph Sünderhauf, Lukas Martini
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

#include <hw/keyboard.h>

#include "keymaps.h"
#include <lib/log.h>
#include <interrupts/interface.h>
#include <filesystems/vfs.h>
#include <lib/datetime.h>
#include <hw/display.h>
#include <hw/pit.h>

// Wait until the command buffer is empty, then send.
#define send(C) while ((inb(0x64) & 0x2)) {}; outb(0x60, C);

// Current modifier keys
struct {
	bool shiftl:1;
	bool shiftr:1;
	bool controll:1;
	bool controlr:1;
	bool alt:1;
	bool super:1;
} modifiers;

void (*focusedFunction)(char);
char* currentKeymap;

// Handle a scancode. Calls the active function
static void handleScancode(uint8 code, uint8 code2)
{
	switch(code)
	{
		case 0x2a: modifiers.shiftl = true; break;
		case 0xaa: modifiers.shiftl = false; break;

		case 0x36: modifiers.shiftr = true; break;
		case 0xb6: modifiers.shiftr = false; break;

		case 0x1d: modifiers.controll = true; break;
		case 0x9d: modifiers.controll = false; break;

		case 0x38: modifiers.alt = true; break;
		case 0xb8: modifiers.alt = false; break;
	}
	

	if( code == 0xe0 && code2 == 0x5b) // super press
		modifiers.super = true;
	if( code == 0xe0 && code2 == 0xdb) // super release
		modifiers.super = false;
	
	if( code == 0xe0 && code2 == 0x49 ) // page up press
		display_scrollUp();
	if( code == 0xe0 && code2 == 0x51 ) // page down press
		display_scrollDown();
	
	if( code2 == 0x1d) // ctrl press
		modifiers.controlr = true;
	if( code2 == 0x9d) // ctrl release
		modifiers.controlr = false;
	
	if( currentKeymap[code] == NULL)
		return;
		
	if( modifiers.shiftl | modifiers.shiftr )
		code = currentKeymap[code + 256];
	else
		code = currentKeymap[code];

	(*focusedFunction)((char)code);
}

// Handles the IRQs we catch
static void handler(cpu_state_t regs)
{
	if(!focusedFunction)
		return;

	static uint8 waitingForEscapeSequence = 0;
	
	// read scancodes
	uint8 code = inb(0x60);
	
	if (code == 0xe0)
		waitingForEscapeSequence = 1; // escape sequence
	else
	{
		if(waitingForEscapeSequence)
		{
			// this is the second scancode to the escape sequence
			handleScancode(0xe0, code);
			waitingForEscapeSequence = 0;
		}
		else
			handleScancode(code, 0); // normal scancode
	}
}

// Take keyboard focus.
void keyboard_takeFocus(void (*func)(char))
{
	focusedFunction = func;
	log("keyboard: Application took focus.\n");
}

// To drop the keyboard focus.
void keyboard_leaveFocus()
{
	focusedFunction = NULL;
}

void keyboard_setLED(int num, bool state)
{
	
}


static void flush()
{
	log("keyboard: Flushing input buffer.\n");
	while(inb(0x64) & 1)
		// read scancode
		log("keyboard: flush: Dropping scancode 0x%x.\n", inb(0x60));
}

// Identify keyboard. You should _not_ call this after initialization.
static char* identify()
{
	/* XT    : Timeout (NOT Port[0x64].Bit[6] = true)
	 * AT    : 0xFA
	 * MF-II : 0xFA 0xAB 0x41
	 */

	send(0xF2);

	// Wait for scancodes
	uint64 startTick = pit_getTickNum();
	while(true)
	{
		if(inb(0x64) & 1)
			break;
		
		uint64 nowTick = pit_getTickNum();
		
		// Still no result after 0.5 seconds
		if((startTick - nowTick) / PIT_RATE >= 0.5)
			return "XT";
	}
	
	uint8 one = inb(0x60);
	uint8 two = inb(0x60);
	uint8 three = inb(0x60);
	
	log("keyboard: identify: one = 0x%x, two = 0x%x, three = 0x%x.\n", one, two, three);

	switch(one)
	{
		case 0xFA:
			if(two == 0xAB && three == 0x41)
				return "MF-II";
			else
				return "AT";
	}
	
	return "Unknown";
}

void keyboard_init()
{
	/* flush input buffer (maybe the user pressed keys before we handled
	 * irqs or set up the idt)
	 */
	flush();
	char* ident = identify();
	log("keyboard: Identified type: %s\n", ident);
	
	send(0xF6); // Set standard values
	send(0xF4); // Activate
	
	flush(); // Flush again
	
	currentKeymap = (char*)&keymap_en;
	interrupts_registerHandler(IRQ1, &handler);
}
