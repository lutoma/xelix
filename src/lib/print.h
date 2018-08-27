#pragma once

/* Copyright © 2011-2018 Lukas Martini
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

#include "generic.h"
#include <stdarg.h>
#include <string.h>
#include <hw/serial.h>
#include <console/console.h>

#define vprintf(fmt, arg...) _vprintf(fmt, arg, puts, putchar)
#define serial_vprintf(fmt, arg...) _vprintf(fmt, arg, serial_print, serial_send)
#define printf __builtin_printf
#define puts __builtin_puts
#define putchar __builtin_putchar

void _vprintf(const char *fmt, void** arg, void (print_cb)(const char*), void (putchar_cb)(const char));
void serial_printf(const char *fmt, ...);

