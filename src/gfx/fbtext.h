#pragma once

/* Copyright © 2019 Lukas Martini
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

#include <stdbool.h>

void gfx_fbtext_init();
void gfx_fbtext_show();
void gfx_fbtext_hide_logo();
void gfx_fbtext_write(uint32_t x, uint32_t y, wchar_t chr, uint32_t col_fg, uint32_t col_bg);
void gfx_fbtext_draw_cursor(uint32_t x, uint32_t y);
void gfx_fbtext_clear(uint32_t x, uint32_t y, uint32_t cols, uint32_t rows);
void gfx_fbtext_scroll(unsigned int num);
