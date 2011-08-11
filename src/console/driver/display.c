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
#include <console/color.h>
#include <memory/kmalloc.h>

static uint16_t* const display_memory = (uint16_t*) 0xB8000;

static void console_driver_display_setCursor(console_info_t *info, uint32_t x, uint32_t y)
{
	uint32_t columns = 80;
	if (info != NULL)
		columns = info->columns;

	uint16_t tmp = y * columns + x;
	outb(0x3d4, 14);
	outb(0x3d5, tmp >> 8);
	outb(0x3d4, 15);
	outb(0x3d5, tmp);
}

static char console_driver_display_packColor(console_info_t *info)
{
	if (info->reverse_video)
		return info->current_color.foreground << 4 | info->current_color.background;

	return info->current_color.background << 4 | info->current_color.foreground;
}

static int console_driver_display_write(console_info_t *info, char c)
{
	char color = console_driver_display_packColor(info);

	if (c == '\n' || c == '\v')
	{
		info->cursor_y++;
		info->cursor_x = 0;
		console_driver_display_setCursor(info, info->cursor_x, info->cursor_y);
		return 0;
	}
	else if (c == '\t')
	{
		info->cursor_x =info->cursor_x + info->tabstop - (info->cursor_x % info->tabstop);
		console_driver_display_setCursor(info, info->cursor_x, info->cursor_y);
		return 0;
	}
	
	if (c == 0x8 || c == 0x7f)
	{
		if (info->cursor_x == 0 && info->cursor_y != 0)
		{
			info->cursor_x = info->columns;
			info->cursor_y -= 1;
		}
		else if (info->cursor_y == 0)
			return 0;

		info->cursor_x--;
		uint16_t *pos = display_memory + info->cursor_y * info->columns + info->cursor_x;
		*pos = (color << 8) | ' ';
		console_driver_display_setCursor(info, info->cursor_x, info->cursor_y);
		return 0;
	}

	while (info->cursor_y >= info->rows)
	{
		memcpy(display_memory, display_memory + 80, info->rows * info->columns * 2);
		memset(display_memory + (info->rows * info->columns) - info->columns, 0, info->columns * 2);
		info->cursor_y--;
	}

	uint16_t *pos = display_memory + info->cursor_y * info->columns + info->cursor_x;
	*pos = (color << 8) | c;

	info->cursor_x++;
	if (info->cursor_x >= info->columns)
	{
		info->cursor_x = info->cursor_x - info->columns;
		info->cursor_y++;
	}

	console_driver_display_setCursor(info, info->cursor_x, info->cursor_y);

	return 1;
}

static int console_driver_display_clear(console_info_t *info)
{
	info->cursor_x = 0;
	info->cursor_y = 0;

  int i = 0;
	char color = console_driver_display_packColor(info);
	while (i < info->columns * info->rows)
		display_memory[i++] = (color << 8) | ' ';

	console_driver_display_setCursor(NULL, 0, 0);
	return 0;
}

console_driver_t *console_driver_display_init(console_driver_t *mem)
{
	if (mem == NULL)
		mem = (console_driver_t *)kmalloc(sizeof(console_driver_t));

	console_driver_display_setCursor(NULL, 0, 0);

	mem->write = console_driver_display_write;
	mem->_clear = console_driver_display_clear;
	mem->setCursor = console_driver_display_setCursor;

	mem->capabilities = CONSOLE_DRV_CAP_CLEAR |
		CONSOLE_DRV_CAP_SET_CURSOR;

	return mem;
}

