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

#ifndef CONSOLE_SCROLL_PAGES
#define CONSOLE_SCROLL_PAGES 8
#endif

static uint16_t* const hardware_memory = (uint16_t*) 0xB8000;
static uint16_t display_memory[25 * 80 * CONSOLE_SCROLL_PAGES];
static uint8_t currentPage;

#define DISP_CHAR(x, y) (display_memory + ( 25 * 80 * (CONSOLE_SCROLL_PAGES - 1)) + x + y * 80)
#define DISP_PAGE(n) (display_memory + 25 * 80 * n)
#define DISP_LAST_PAGE (display_memory + (25 * 80 * (CONSOLE_SCROLL_PAGES - 1)))
#define DISP_LAST_LINE (display_memory + 25 * 80 * CONSOLE_SCROLL_PAGES - 80)
#define DISP_BYTES (25 * 80 * 2 * CONSOLE_SCROLL_PAGES)
#define DISP_PAGE_SIZE ( 25 * 80 * 2)
#define DISP_LINE_SIZE ( 80 * 2)
#define DISP_CHAR_SIZE ( 2 )

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
		goto return_write;
		return 0;
	}
	else if (c == '\t')
	{
		uint32_t columns = info->tabstop - (info->cursor_x % info->tabstop);
		info->cursor_x = info->cursor_x + columns;
		if (info->cursor_x >= info->columns)
			info->cursor_x = info->columns - 1;
		goto return_write;
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
		uint16_t *pos = DISP_CHAR(info->cursor_x, info->cursor_y);
		*pos = (color << 8) | ' ';
		goto return_write;
	}

	while (info->cursor_y >= info->rows)
	{
		memcpy(DISP_PAGE(0), DISP_PAGE(0) + 80, DISP_BYTES - DISP_LINE_SIZE);
		memset(DISP_LAST_LINE, 0, DISP_LINE_SIZE);
		info->cursor_y--;
	}

	uint16_t *pos = DISP_CHAR(info->cursor_x, info->cursor_y);
	*pos = (color << 8) | c;

	info->cursor_x++;
	if (info->cursor_x >= info->columns)
	{
		info->cursor_x = info->cursor_x - info->columns;
		info->cursor_y++;
	}

return_write:
	console_driver_display_setCursor(info, info->cursor_x, info->cursor_y);
	memcpy(hardware_memory, DISP_LAST_PAGE, DISP_PAGE_SIZE);

	return 1;
}

static int console_driver_display_clear(console_info_t *info)
{
	memcpy(DISP_PAGE(0), DISP_PAGE(1), DISP_BYTES - DISP_PAGE_SIZE);
	memset(DISP_LAST_PAGE, 0, DISP_PAGE_SIZE);
	info->cursor_x = 0;
	info->cursor_y = 0;
	console_driver_display_setCursor(NULL, 0, 0);
	return 0;
}

static int console_driver_display_scroll(console_info_t *info, int32_t page)
{
	if (page == 0)
		return 0;

	currentPage = currentPage + page;
	uint32_t offset = CONSOLE_SCROLL_PAGES - (currentPage % 8) - 1;
	uint16_t *newPage = DISP_PAGE(offset);

	memcpy(hardware_memory, newPage, DISP_PAGE_SIZE);
	return offset;
}

console_driver_t *console_driver_display_init(console_driver_t *mem)
{
	if (mem == NULL)
		mem = (console_driver_t *)kmalloc(sizeof(console_driver_t));

	console_driver_display_setCursor(NULL, 0, 0);

	mem->write = console_driver_display_write;
	mem->_clear = console_driver_display_clear;
	mem->setCursor = console_driver_display_setCursor;
	mem->scroll = console_driver_display_scroll;

	mem->capabilities = CONSOLE_DRV_CAP_CLEAR |
		CONSOLE_DRV_CAP_SET_CURSOR |
		CONSOLE_DRV_CAP_SCROLL;

	return mem;
}

