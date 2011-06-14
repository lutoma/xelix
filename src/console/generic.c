/* generic.c: Generic terminal access
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

#include <console/interface.h>
#define GET_CONSOLE(console) if (console == NULL)\
	console = &default_console; \
	if (console == NULL) \
		panic("Requested operation on undefined console");

console_t default_console;

#include <console/driver/display.h>
#include <memory/kmalloc.h>

void console_init()
{
	default_console.info.rows = 25;
	default_console.info.columns = 80;
	default_console.info.cursor_x = 0;
	default_console.info.cursor_y = 0;

	default_console.input_driver = NULL;
	default_console.input_filter = NULL;
	default_console.output_filter = NULL;

	default_console.output_driver = (console_driver_t *)kmalloc(sizeof(console_driver_t));
	console_driver_display_init(default_console.output_driver);
}

void console_clear(console_t *console)
{
	GET_CONSOLE(console);

	if (console->output_driver->capabilities & CONSOLE_DRV_CAP_CLEAR)
		console->output_driver->_clear(&console->info);
}

size_t console_write(console_t *console, const char *buffer, size_t length)
{
	GET_CONSOLE(console);

	console_filter_t *filter;
	int i = 0;
	size_t written = 0;
	char c;
	int retval;
	while (i < length)
	{
		c = buffer[i++];
		filter = console->output_filter;
		while (filter != NULL)
		{
			c = filter->callback(c, &(console->info));
			filter = filter->next;
		}

		retval = console->output_driver->write(&console->info, c);
		if (retval == -1)
			break;

		written += retval;
	}

	return written;
}

size_t console_read(console_t *console, char *buffer, size_t length)
{
	GET_CONSOLE(console);

	console_filter_t *filter;
	int i = 0;
	size_t read = 0;

	while (i < length)
	{
		buffer[i] = console->output_driver->read(&console->info);

		if (buffer[i] == -1)
			break;

		filter = console->input_filter;
		while (filter != NULL)
		{
			buffer[i] = filter->callback(buffer[i], &console->info);
			filter = filter->next;
		}

		read++;
		i++;
	}

	return read;
}
