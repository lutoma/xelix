/* serial.c: Console driver for the serial port
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

#include <console/driver/serial.h>
#include <hw/serial.h>
#include <memory/kmalloc.h>

static void console_driver_serial_setCursor(console_info_t *info, uint32_t x, uint32_t y)
{
}

static int console_driver_serial_write(console_info_t *info, char c)
{
	if (c == 0x7f)
		c = 0x8;

	char cs[2];
	cs[0] = c;
	cs[1] = 0;

	serial_print(cs);

	return 1;
}

static char console_driver_serial_read(console_info_t *info)
{
	return serial_recv();
}

static int console_driver_serial_clear(console_info_t *info)
{
	serial_print("\e[H\e[2J");
	return 0;
}

console_driver_t *console_driver_serial_init(console_driver_t *driver)
{
	if (driver == NULL)
		driver = (console_driver_t*)kmalloc(sizeof(console_driver_t));

	driver->read = console_driver_serial_read;
	driver->write = console_driver_serial_write;
	driver->_clear = console_driver_serial_clear;
	driver->setCursor = console_driver_serial_setCursor;
	driver->capabilities = CONSOLE_DRV_CAP_CLEAR |
		CONSOLE_DRV_CAP_SET_CURSOR;

	return driver;
}
