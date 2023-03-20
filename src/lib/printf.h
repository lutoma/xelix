#pragma once

/* Copyright © 2019-2022 Lukas Martini
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

#include <stdarg.h>
#include <stddef.h>
#include <tty/serial.h>
#include <tty/term.h>
#include <tty/console.h>

extern void fbtext_write_char(char);
#define PRINTF_ALIAS_STANDARD_FUNCTION_NAMES 1

static inline void putchar_(char c) {
    if(term_console) {
        term_write(term_console, &c, 1);
    }
}

#include <printf/printf.h>
