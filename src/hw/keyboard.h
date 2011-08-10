#pragma once

/* Copyright © 2010 Christoph Sünderhauf
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

#include <lib/generic.h>

// Wait until the command buffer is empty, then send.
#define keyboard_sendKeyboard(C) while ((inb(0x64) & 0x2)) {}; outb(0x60, C);
#define keyboard_sendKBC(C) while ((inb(0x64) & 0x2)) {}; outb(0x64, C);

void keyboard_init(); // irqs have to be set up first!
void keyboard_takeFocus(void (*func)(uint16_t));
void keyboard_leaveFocus();
char keyboard_codeToChar(uint16_t code);
