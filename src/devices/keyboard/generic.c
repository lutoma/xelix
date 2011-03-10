/* generic.c: A generic & simple keyboard driver
 * Copyright © 2010 Christoph Sünderhauf
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

#include "interface.h"

#include "keymaps.h"
#include <lib/log.h>
#include <interrupts/interface.h>
#include <filesystems/vfs.h>

// Current modifier keys
struct {
	int shiftl:1;
	int shiftr:1;
	int controll:1;
	int controlr:1;
	int alt:1;
	int super:1;
} modifiers;

void (*focusedFunction)(char);
char* currentKeymap;

// Handle a scancode. Calls the active function
static void handleScancode(uint8 code, uint8 code2)
{	
	if(!focusedFunction)
		return;
	
	if( code==0x2a) // shift press
		modifiers.shiftl=1;
	if( code==0xaa) // shift release
		modifiers.shiftl=0;
	if( code==0x36) // shift press
		modifiers.shiftr=1;
	if( code==0x36+0x80) // shift release
		modifiers.shiftr=0;
	if( code==0x1d) // ctrl press
		modifiers.controll=1;
	if( code==0x9d) // control release
		modifiers.controll=0;
	if( code2==0x1d) // ctrl press
		modifiers.controlr=1;
	if( code2==0x9d) // control release
		modifiers.controlr=0;
	if( code==0x38) // alt press
		modifiers.alt = 1;
	if( code==0xb8) // alt release
		modifiers.alt=0;
	if( code==0xe0 && code2==0x5b) // super press
		modifiers.super=1;
	if( code==0xe0 && code2==0xdb) // super release
		modifiers.super=0;
	
	if( code==0xe0 && code2==0x49 ) // page up press
		display_scrollUp();
	if( code==0xe0 && code2==0x51 ) // page down press
		display_scrollDown();
	
	if( currentKeymap[code] == NULL)
		return;
		
	if( modifiers.shiftl | modifiers.shiftr )
		code = currentKeymap[code + 256];
	else
		code = currentKeymap[code];

	(*focusedFunction)((char)code);
}

// Handles the IRQs we catch
static void handler(registers_t regs)
{
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

void keyboard_init()
{
	currentKeymap = &keymap_en;
	interrupt_registerHandler(IRQ1, &handler);
	
	// flush input buffer (maybe the user pressed keys before we handle irqs or set up the idt)
	// see also: http://forum.osdev.org/viewtopic.php?p=176249
	while(inb(0x64) & 1)
		inb(0x60); // read scancode
}
