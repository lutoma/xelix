#pragma once

/* Copyright © 2010 Benjamin Richter
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <printf.h>

#define serial_printf(fmt, args...) fctprintf(serial_send, NULL, fmt, ## args)

void serial_init(void);
void serial_init2(void);
char serial_recv(void);
void serial_send(const char c, void* unused);
