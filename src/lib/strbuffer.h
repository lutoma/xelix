#pragma once

/* Copyright © 2011 Fritz Grimpen
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

#include <lib/generic.h>

typedef struct {
	char *data;

	uint32_t allocated;
	uint32_t length;
} strbuffer_t;

strbuffer_t *strbuffer_new(uint32_t allocation);
strbuffer_t *strbuffer_resize(strbuffer_t *buffer, uint32_t new_allocation);
strbuffer_t *strbuffer_append(strbuffer_t *buffer, char c);
//strbuffer_t *strbuffer_prepend(strbuffer_t *buffer, char c);
strbuffer_t *strbuffer_concat(strbuffer_t *buffer1, strbuffer_t *buffer2);
void strbuffer_clear(strbuffer_t *buffer);

char strbuffer_chr(strbuffer_t *buffer, uint32_t offset);
char strbuffer_last(strbuffer_t *buffer);
// Unimplemented at the moment
//strbuffer_t *strbuffer_substr(strbuffer_t *buffer, uint32_t offset, uint32_t length);
