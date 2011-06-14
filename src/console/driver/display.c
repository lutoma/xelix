/* display.c: Console driver for the generic display
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

#include <console/driver.h>
#include <memory/kmalloc.h>

static uint16_t* const display_memory = (uint16_t*) 0xB8000;

static int console_driver_display_write(console_info_t *info, char c)
{
	if (c == '\n')
	{
		info->cursor_x++;
		info->cursor_y = 0;
		return 0;
	}

	if (c == '\t')
	{
		info->cursor_y += 4;
		return 4;
	}

	while (info->cursor_x >= info->rows)
	{
		memcpy(display_memory, display_memory + 80, info->rows * info->columns * 2);
		memset(display_memory + (info->rows * info->columns) - info->columns, 0, info->columns * 2);
		info->cursor_y--;
	}

	uint16_t *pos = display_memory + info->cursor_x * info->columns + info->cursor_y;
	*pos = (15 << 8) | c;

	info->cursor_y++;
	if (info->cursor_y >= info->columns)
	{
		info->cursor_y = info->cursor_y - info->columns;
		info->cursor_x++;
	}

	return 1;
}

static int console_driver_display_clear(console_info_t *info)
{
	memset(display_memory, 0, info->rows * info->columns * 2);
	info->cursor_x = 0;
	info->cursor_y = 0;
	return 0;
}

console_driver_t *console_driver_display_init(console_driver_t *mem)
{
	if (mem == NULL)
		mem = (console_driver_t *)kmalloc(sizeof(console_driver_t));

	mem->write = console_driver_display_write;
	mem->_clear = console_driver_display_clear;
	mem->capabilities = CONSOLE_DRV_CAP_CLEAR;

	return mem;
}

