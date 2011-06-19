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

static char console_filter_ecma48_writeCallback(char c, console_info_t *info, int (*_write)(console_info_t *, char))
{
	return c;
}

console_filter_t *console_filter_ecma48_init(console_filter_t *filter)
{
	if (filter == NULL)
		filter = (console_filter_t *)kmalloc(sizeof(console_filter_t));

	filter->callback = NULL;
	filter->read_callback = NULL;
	filter->write_callback = console_filter_ecma48_writeCallback;

	filter->next = NULL;

	return filter;
}
