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

#include <console/filter/ecma48.h>
#include <memory/kmalloc.h>
#include <lib/dict/dict.h>
#include <lib/strbuffer.h>

static dict_t *buffer_dictionary = NULL;

static void processControlSequence(console_info_t *info, strbuffer_t *buffer)
{
}

static char console_filter_ecma48_writeCallback(char c, console_info_t *info, int (*_write)(console_info_t *, char))
{
	bool discard = 0;
	bool complete = 0;

	if (buffer_dictionary == NULL)
		buffer_dictionary = dict_new();

	strbuffer_t *buffer = dict_get(buffer_dictionary, info);
	if (buffer == (void *)-1)
	{
		buffer = strbuffer_new(0);
		dict_set(buffer_dictionary, info, buffer);
	}

	if (buffer->length == 0)
	{
		if (c == 27)
			strbuffer_append(buffer, c);
		else
			discard = 1;
	}
	else if (strbuffer_last(buffer) == 27)
	{
		if (c == 92)
			strbuffer_append(buffer, c);
		else
			discard = 1;
	}
	else
	{
		if ((c >= 48 && c <= 57) || c == ';')
			strbuffer_append(buffer, c);
		else if (c == 'm')
		{
			strbuffer_append(buffer, c);
			complete = 1;
		}
		else
			discard = 1;
	}

	if (complete)
	{
		processControlSequence(info, buffer);
		discard = 1;
	}

	if (!discard)
	{
		return 0;
	}

	if (!complete)
	{
		int i = 0;
		while ( i < buffer->length )
		{
			_write(info, strbuffer_chr(buffer, i++));
		}
	}

	strbuffer_clear(buffer);

	return c;
}

console_filter_t *console_filter_ecma48_init(console_filter_t *filter)
{
	if (buffer_dictionary == NULL)
		buffer_dictionary = dict_new();

	if (filter == NULL)
		filter = (console_filter_t *)kmalloc(sizeof(console_filter_t));

	filter->callback = NULL;
	filter->read_callback = NULL;
	filter->write_callback = console_filter_ecma48_writeCallback;

	filter->next = NULL;

	return filter;
}
