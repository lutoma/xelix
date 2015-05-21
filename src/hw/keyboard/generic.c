/* generic.c: A generic keyboard driver. Should work for every PS2
 * keyboard and most USB keyboards (Depends on BIOS legacy support)
 *
 * Copyright © 2010 Christoph Sünderhauf, Lukas Martini
 * Copyright © 2011 Lukas Martini, Fritz Grimpen
 * Copyright © 2012 Lukas Martini
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

#include "keymaps.h"
#include <console/info.h>
#include <hw/keyboard.h>
#include <memory/kmalloc.h>
#include <console/interface.h>
#include <lib/log.h>
#include <lib/string.h>
#include <interrupts/interface.h>
#include <hw/display.h>
#include <hw/pit.h>

#include <lib/dict.h>

struct keyboard_buffer {
	char* data;
	uint32_t size;
	uint32_t offset;
};

static struct keyboard_buffer keyboard_buffer = {
	.data = NULL,
	.size = 0,
	.offset = 0
};

// Current modifier keys
static console_modifiers_t modifiers = {
	.shift_left = false,
	.shift_right = false,
	.control_left = false,
	.control_right = false,
	.alt = false,
	.super = false
};

static dict_t* dictionary;
static char* current_keymap;

int keyboard_setlayout(char* layoutname)
{
	void* retval = dict_get(dictionary, layoutname);
	if((int)retval != -1)
		current_keymap = (char*)retval;
	else
		return -1;

	return 1;
}

static void flush()
{
	log(LOG_INFO, "keyboard: Flushing input buffer.\n");
	while(inb(0x64) & 1)
		// read scancode
		log(LOG_INFO, "keyboard: flush: Dropping scancode 0x%x.\n", inb(0x60));
}

#if 0
// Identify keyboard. You should _not_ call this after initialization.
static char* identify()
{
	/* XT    : Timeout (NOT Port[0x64].Bit[6] = true)
	 * AT    : 0xFA
	 * MF-II : 0xFA 0xAB 0x41
	 */

	keyboard_sendKeyboard(0xF2);

	// Wait for scancodes
	uint64_t startTick = pit_getTickNum();
	while(true)
	{
		if(inb(0x64) & 1)
			break;

		uint64_t nowTick = pit_getTickNum();

		// Still no result after 0.5 seconds
		if((startTick - nowTick) / PIT_RATE >= 0.5)
			return "XT";
	}

	uint8_t one = inb(0x60);
	uint8_t two = inb(0x60);
	uint8_t three = inb(0x60);

	log(LOG_INFO, "keyboard: identify: one = 0x%x, two = 0x%x, three = 0x%x.\n", one, two, three);

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
#endif

// Handle a scancode. Calls the active function
static void handleScancode(uint8_t code, uint8_t code2)
{
	switch(code)
	{
		case 0x2a: modifiers.shift_left = true; break;
		case 0xaa: modifiers.shift_left = false; break;

		case 0x36: modifiers.shift_right = true; break;
		case 0xb6: modifiers.shift_right = false; break;

		case 0x1d: modifiers.control_left = true; break;
		case 0x9d: modifiers.control_left = false; break;

		case 0x38: modifiers.alt = true; break;
		case 0xb8: modifiers.alt = false; break;
	}

	if( code == 0xe0 && code2 == 0x5b) // super press
		modifiers.super = true;
	if( code == 0xe0 && code2 == 0xdb) // super release
		modifiers.super = false;

	if( code == 0xe0 && code2 == 0x49 ) // page up press
		console_scroll(NULL, 1);
	if( code == 0xe0 && code2 == 0x51 ) // page down press
		console_scroll(NULL, -1);

	if( code2 == 0x1d) // ctrl press
		modifiers.control_right = true;
	if( code2 == 0x9d) // ctrl release
		modifiers.control_right = false;

	uint16_t dcode = code;
	if( modifiers.shift_left | modifiers.shift_right )
		dcode += 256;

	if(code > 512)
		return;

	char c = current_keymap[dcode];
	if(code > 512 || c == 0)
		return;

    // 0x8 is backspace, in which case we delete a byte from the buffer
	if (c == 0x8 && keyboard_buffer.offset > 0)
	{
		if (keyboard_buffer.size == 0 || keyboard_buffer.data == NULL)
			return;

        // XXX: is it really necessary to actually shrink the buffer?
		char* new_buffer = (char*)kmalloc(sizeof(char) * (keyboard_buffer.size - 1));
		memcpy(new_buffer, keyboard_buffer.data, keyboard_buffer.size - 1);
		kfree(keyboard_buffer.data);
		keyboard_buffer.data = new_buffer;
		keyboard_buffer.size--;
		keyboard_buffer.offset--;
		return;
	}

	if (keyboard_buffer.size <= keyboard_buffer.offset)
	{
		char* new_buffer = (char*)kmalloc(sizeof(char) * (keyboard_buffer.size + 1));
		if (keyboard_buffer.data != NULL)
		{
			memcpy(new_buffer, keyboard_buffer.data, keyboard_buffer.size);
			kfree(keyboard_buffer.data);
		}
		keyboard_buffer.size++;
		keyboard_buffer.data = new_buffer;
	}

	keyboard_buffer.data[keyboard_buffer.offset++] = c;
}

// Handles the IRQs we catch
static void irq_handler(cpu_state_t* regs)
{
	// Escape sequences consist of two scancodes: One first that tells us
	// we're now in an escape sequence, and the second one with the actual
	// sequence.
	static bool escape_seq_wait;

	// Read scancode
	uint8_t code = inb(0x60);

	// Escape sequence scancode
	if(unlikely(code == 0xe0))
	{
		escape_seq_wait = true;
		return;
	}

	if(unlikely(escape_seq_wait))
	{
		// This is the second scancode to the escape sequence
		escape_seq_wait = false;
		handleScancode(0xe0, code);
	}
	else
		handleScancode(code, 0); // normal scancode
}

static console_read_t* console_driver_keyboard_read(console_info_t *info)
{
	if (keyboard_buffer.size == 0 || keyboard_buffer.offset == 0)
		return NULL;

	if (keyboard_buffer.size == 1)
	{
		keyboard_buffer.offset = 0;
	}
	else
	{
		char* new_buffer = (char*)kmalloc(sizeof(char) * (keyboard_buffer.size - 1));
		memcpy(new_buffer, keyboard_buffer.data + 1, keyboard_buffer.size - 1);
		kfree(keyboard_buffer.data);
		keyboard_buffer.size--;
		keyboard_buffer.offset--;
		keyboard_buffer.data = new_buffer;
	}

	console_read_t* read = kmalloc(sizeof(console_read_t));
	read->character = keyboard_buffer.data[0];
	read->modifiers = &modifiers;

	return read;
}

console_driver_t* console_driver_keyboard_init(console_driver_t* driver)
{
	/* flush input buffer (maybe the user pressed keys before we handled
	 * irqs or set up the idt)
	 */
	flush();

	// This is currently unused, might as well comment it out…
	// char* ident = identify();
	// log(LOG_INFO, "keyboard: Identified type: %s\n", ident);

	// Reset to default values
	keyboard_sendKeyboard(0xF6);

	// Setting repeat rate to maximum
	keyboard_sendKeyboard(0xF3); // CMD
	keyboard_sendKeyboard(0b00000000); // Value

	// Activate
	keyboard_sendKeyboard(0xF4);

	flush(); // Flush again

    // load keymaps
    dictionary = dict_new(1);
    dict_set(dictionary, "en", keymap_en);
    dict_set(dictionary, "de", keymap_de);
	current_keymap = keymap_en;

	interrupts_registerHandler(IRQ1, &irq_handler);

	if(driver == NULL)
		driver = (console_driver_t*)kmalloc(sizeof(console_driver_t));

	driver->read = console_driver_keyboard_read;
	return driver;
}
