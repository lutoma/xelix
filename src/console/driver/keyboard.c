/* keyboard.c: Console driver for the generic keyboard
 * Copyright © 2011 Fritz Grimpen
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <console/driver/keyboard.h>
#include <console/info.h>
#include <hw/keyboard.h>
#include <memory/kmalloc.h>

struct keyboard_buffer {
	char *data;
	uint32_t size;
	uint32_t offset;
};

static struct keyboard_buffer keyboard_buffer = {
	.data = NULL,
	.size = 0,
	.offset = 0
};

static void console_driver_keyboard_focus(uint8_t code)
{
	char c = keyboard_codeToChar(code);

	if (c == NULL)
		return;

	if (c == 0x8 && keyboard_buffer.offset > 0)
	{
		if (keyboard_buffer.size == 0 || keyboard_buffer.data == NULL)
			return;

		char *new_buffer = (char *)kmalloc(sizeof(char) * (keyboard_buffer.size - 1));
		memcpy(new_buffer, keyboard_buffer.data, keyboard_buffer.size - 1);
		kfree(keyboard_buffer.data);
		keyboard_buffer.data = new_buffer;
		keyboard_buffer.size--;
		keyboard_buffer.offset--;
		return;
	}

	if (keyboard_buffer.size <= keyboard_buffer.offset)
	{
		char *new_buffer = (char *)kmalloc(sizeof(char) * (keyboard_buffer.size + 1));
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

static char console_driver_keyboard_read(console_info_t *info)
{
	if (keyboard_buffer.size == 0 || keyboard_buffer.offset == 0)
		return 0;

	char retval = keyboard_buffer.data[0];

	if (keyboard_buffer.size == 1)
	{
		keyboard_buffer.offset = 0;
	}
	else
	{
		char *new_buffer = (char *)kmalloc(sizeof(char) * (keyboard_buffer.size - 1));
		memcpy(new_buffer, keyboard_buffer.data + 1, keyboard_buffer.size - 1);
		kfree(keyboard_buffer.data);
		keyboard_buffer.size--;
		keyboard_buffer.offset--;
		keyboard_buffer.data = new_buffer;
	}

	return retval;
}

console_driver_t *console_driver_keyboard_init(console_driver_t *driver)
{
	keyboard_takeFocus(console_driver_keyboard_focus);

	if (driver == NULL)
		driver = (console_driver_t *)kmalloc(sizeof(console_driver_t));

	driver->read = console_driver_keyboard_read;

	return driver;
}

