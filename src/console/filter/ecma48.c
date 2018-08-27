/* ecma48.c: Filter for ECMA-48 Escape Sequences
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

#include <stdlib.h>
#include <console/filter/ecma48.h>
#include <memory/kmalloc.h>
#include <dict.h>
#include <strbuffer/strbuffer.h>

static dict_t* buffer_dictionary = NULL;

static void processColorSequence(console_info_t* info, strbuffer_t* buffer)
{
	int i = 2;
	char c = 0;
	char old_c = 0;
	while (i < buffer->length)
	{
		old_c = c;
		c = strbuffer_chr(buffer, i++);
		if (c == 'm' || (c >= '0' && c <= '9' && old_c >= '0' && old_c <= '9'))
			continue;

		if (c == '0')
		{
			c = strbuffer_chr(buffer, i++);
			info->current_color = info->default_color;
		}
		else if (c == '1')
		{
			c = strbuffer_chr(buffer, i++);
			info->bold = 1;
		}
		else if (c == '2')
		{
			c = strbuffer_chr(buffer, i++);
			if (c == '5')
				info->blink = 0;
			else if (c == '4')
				info->underline = 0;
			else if (c == '7')
			{
				info->reverse_video = 0;
			}
		}
		else if (c == '3')
		{
			c = strbuffer_chr(buffer, i++);
			if (c == '0')
				info->current_color.foreground = CONSOLE_COLOR_BLACK;
			else if (c == '1')
				info->current_color.foreground = CONSOLE_COLOR_RED;
			else if (c == '2')
				info->current_color.foreground = CONSOLE_COLOR_GREEN;
			else if (c == '3')
				info->current_color.foreground = CONSOLE_COLOR_BROWN;
			else if (c == '4')
				info->current_color.foreground = CONSOLE_COLOR_BLUE;
			else if (c == '5')
				info->current_color.foreground = CONSOLE_COLOR_MAGENTA;
			else if (c == '6')
				info->current_color.foreground = CONSOLE_COLOR_CYAN;
			else if (c == '7')
				info->current_color.foreground = CONSOLE_COLOR_WHITE;
			else if (c == '8' || c == '9')
				info->current_color.foreground = info->default_color.foreground;
		}
		else if (c == '4')
		{
			c = strbuffer_chr(buffer, i++);
			if (c == '0')
				info->current_color.background = CONSOLE_COLOR_BLACK;
			else if (c == '1')
				info->current_color.background = CONSOLE_COLOR_RED;
			else if (c == '2')
				info->current_color.background = CONSOLE_COLOR_GREEN;
			else if (c == '3')
				info->current_color.background = CONSOLE_COLOR_BROWN;
			else if (c == '4')
				info->current_color.background = CONSOLE_COLOR_BLUE;
			else if (c == '5')
				info->current_color.background = CONSOLE_COLOR_MAGENTA;
			else if (c == '6')
				info->current_color.background = CONSOLE_COLOR_CYAN;
			else if (c == '7')
				info->current_color.background = CONSOLE_COLOR_WHITE;
			else if (c == '9')
				info->current_color.background = info->default_color.background;
		}
		else if (c == '7')
		{
			info->reverse_video = 1;
		}
	}
}

static void processControlSequence(console_info_t* info, strbuffer_t* buffer, console_driver_t* output)
{
	int prefix = atoi(buffer->data + 2);

	switch (strbuffer_last(buffer))
	{
		case 'm':
			processColorSequence(info, buffer);
			break;
		case 'A':
			if (prefix == 0)
				prefix = 1;
			while (info->cursor_y != 0 && prefix != 0)
			{
				info->cursor_y--;
				prefix--;
			}
			break;
		case 'B':
			if (prefix == 0)
				prefix = 1;
			while (prefix != 0)
			{
				info->cursor_y++;
				prefix--;
			}
			break;
		case 'C':
			if (prefix == 0)
				prefix = 1;
			while (prefix != 0)
			{
				info->cursor_x++;
				prefix--;
			}
			break;
		case 'D':
			if (prefix == 0)
				prefix = 1;
			while (info->cursor_x != 0 && prefix != 0)
			{
				info->cursor_x--;
				prefix--;
			}
			break;
		case 'H':
			info->cursor_x = 0;
			info->cursor_y = 0;
			break;
		case 'J':
			if (output->capabilities & CONSOLE_DRV_CAP_CLEAR)
				output->_clear(info);
			break;
		default:
			{
				int i = 0;
				while (i < buffer->length)
					output->write(info, strbuffer_chr(buffer, i++));
			}
	}
}

static char console_filter_ecma48_writeCallback(char c, console_info_t* info, console_driver_t* output)
{
	bool discard = 0;
	bool complete = 0;

	if (buffer_dictionary == NULL)
		buffer_dictionary = dict_new(false);

	strbuffer_t* buffer = dict_get(buffer_dictionary, info);
	if (buffer == (void*)-1)
	{
		buffer = strbuffer_new(0);
		dict_set(buffer_dictionary, info, buffer);
	}

	if (buffer->length == 0)
	{
		if (c == 27)
			strbuffer_append(buffer, c);
		else if (c == 0x9b)
		{
			strbuffer_append(buffer, 27);
			strbuffer_append(buffer, '[');
		}
		else
			discard = 1;
	}
	else if (strbuffer_last(buffer) == 27)
	{
		if (c == '[')
			strbuffer_append(buffer, c);
		else
			discard = 1;
	}
	else
	{
		if ((c >= '0' && c <= '9') || c == ';')
			strbuffer_append(buffer, c);
		else if (c == 'm' || c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'H' || c == 'J')
		{
			strbuffer_append(buffer, c);
			complete = 1;
		}
		else
			discard = 1;
	}

	if (complete == 1)
	{
		processControlSequence(info, buffer, output);
		strbuffer_clear(buffer);
		return 0;
	}

	if (discard == 0)
		return 0;

	if (complete == 0)
	{
		int i = 0;
		while ( i < buffer->length )
		{
			output->write(info, strbuffer_chr(buffer, i++));
		}
	}

	strbuffer_clear(buffer);

	return c;
}

console_filter_t* console_filter_ecma48_init(console_filter_t* filter)
{
	if (buffer_dictionary == NULL)
		buffer_dictionary = dict_new(false);

	if (filter == NULL)
		filter = (console_filter_t*)kmalloc(sizeof(console_filter_t));

	filter->callback = NULL;
	filter->read_callback = NULL;
	filter->write_callback = console_filter_ecma48_writeCallback;

	filter->next = NULL;

	return filter;
}
