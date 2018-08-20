/* generic.c: Generic terminal access
 * Copyright © 2011 Fritz Grimpen, Lukas Martini
 * Copyright © 2013-2018 Lukas Martini
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

#include <print.h>
#include <spinlock.h>
#include <console/console.h>
#include <fs/sysfs.h>

#define GET_CONSOLE(console, else) if (console == NULL)\
	console = default_console; \
	if (console == NULL) \
		else

console_t* default_console = NULL;
static spinlock_t console_write_lock = SPINLOCK_UNLOCKED;

#include <console/driver/display.h>
#include <hw/keyboard.h>
#ifdef CONSOLE_USE_SERIAL
# include <console/driver/serial.h>
#	define CONSOLE_NO_ECMA48
#endif
#ifndef CONSOLE_NO_ECMA48
#	include <console/filter/ecma48.h>
#endif
#include <console/filter/vt100.h>
#include <memory/kmalloc.h>
#ifndef CONSOLE_DEFAULT_FG
#	define CONSOLE_DEFAULT_FG CONSOLE_COLOR_LGREY
#endif
#ifndef CONSOLE_DEFAULT_BG
#	define CONSOLE_DEFAULT_BG CONSOLE_COLOR_BLACK
#endif

void console_clear(console_t* console)
{
	GET_CONSOLE(console, return);

	if (console->output_driver->capabilities & CONSOLE_DRV_CAP_CLEAR)
		console->output_driver->_clear(&console->info);
}

size_t console_scroll(console_t *console, int32_t pages)
{
	GET_CONSOLE(console, return 0);

	if (console->output_driver->capabilities & CONSOLE_DRV_CAP_SCROLL)
		return console->output_driver->scroll(&console->info, pages);

	return 0;
}


/* The length parameter should be a size_t instead of int32_t, however when you
 * change it to an unsigned type, the console framework breaks badly. Obviously
 * something is very wrong here resp. in the calls to console_write.
 */
size_t console_write(console_t *console, const char *buffer, int32_t length)
{
	GET_CONSOLE(console, return 0);

	// We don't want writes from multiple tasks all muddled together
	if(!spinlock_get(&console_write_lock, 50)) {
		return 0;
	}

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
			if (filter->callback != NULL)
				c = filter->callback(c, &(console->info), console->input_driver, console->output_driver);
			if (filter->write_callback != NULL)
				c = filter->write_callback(c, &(console->info), console->output_driver);
			if (c == 0)
				break;
			filter = filter->next;
		}

		if (c == 0)
		{
			continue;
		}

		retval = console->output_driver->write(&console->info, c);
		if (retval == -1)
			break;

		written += retval;
	}

	spinlock_release(&console_write_lock);
	return written;
}

size_t console_read(console_t* console, char* buffer, size_t length)
{
	GET_CONSOLE(console, return 0);

	console_filter_t* filter;
	int i = 0;
	size_t read = 0;

	while (i < length)
	{
		console_read_t* input = console->input_driver->read(&console->info);

		if(unlikely(!input))
		{
			if (console->info.nonblocking)
				i++;
			continue;
		}

		// Save pointers elsewhere so we can free the return struct
		buffer[i] = input->character;
		console_modifiers_t* current_modifiers = input->modifiers;
		kfree(input);

		// Backspace
		if(unlikely(buffer[i] == 0x8 || buffer[i] == 0x7f) && console->info.handle_backspace)
		{
			if(read <= 0)
				continue;
			console_write(console, (char*)&buffer[i], 1);

			read--;
			i--;
			continue;
		}

		// Check for ^D and return immediately if found
		if((current_modifiers->control_left || current_modifiers->control_right)
			&& (buffer[i] == 'd' || buffer[i] == 'D'))
			return read;

		if (console->info.auto_echo && likely(buffer[i] != 0x8 && buffer[i] != 0x7f))
			console_write(console, (char*)&buffer[i], 1);

		if(unlikely(buffer[i] == '\n' || buffer[i] == '\r'))
			return ++read;

		filter = console->input_filter;
		while (filter != NULL)
		{
			if (filter->callback != NULL)
				buffer[i] = filter->callback(buffer[i], &console->info, console->input_driver, console->output_driver);
			if (filter->read_callback != NULL)
				buffer[i] = filter->read_callback(buffer[i], &console->info, console->input_driver);
			filter = filter->next;
		}

		read++;
		i++;
	}

	return read;
}

void console_init()
{
	if (default_console == NULL)
		default_console = (console_t*)kmalloc(sizeof(console_t));

	default_console->info.rows = 25;
	default_console->info.columns = 80;
	default_console->info.tabstop = 8;
	default_console->info.cursor_x = 0;
	default_console->info.cursor_y = 0;
	default_console->info.newline_mode = true;
	default_console->info.auto_echo = true;
	default_console->info.handle_backspace = true;

	default_console->input_filter = NULL;
	default_console->output_filter = NULL;

	default_console->input_driver = (console_driver_t*)kmalloc(sizeof(console_driver_t));
	default_console->output_driver = (console_driver_t*)kmalloc(sizeof(console_driver_t));

#	ifdef CONSOLE_USE_SERIAL
	console_driver_serial_init(default_console->output_driver);
	console_driver_serial_init(default_console->input_driver);
#	else
	console_driver_display_init(default_console->output_driver);
	console_driver_keyboard_init(default_console->input_driver);
# endif

	default_console->output_filter = console_filter_vt100_init(NULL);

#	ifndef CONSOLE_NO_ECMA48
	default_console->output_filter->next = console_filter_ecma48_init(NULL);
#	endif

	default_console->info.default_color.background = CONSOLE_DEFAULT_BG;
	default_console->info.default_color.foreground = CONSOLE_DEFAULT_FG;

	default_console->info.current_color = default_console->info.default_color;

	default_console->info.nonblocking = false;
	default_console->info.reverse_video = false;
	default_console->info.bold = false;
	default_console->info.blink = false;
	default_console->info.underline = false;

	console_clear(NULL);

	sysfs_add_dev("stdin", NULL);
	sysfs_add_dev("stdout", NULL);
	sysfs_add_dev("stderr", NULL);
}
