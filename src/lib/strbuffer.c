/* strbuffer.c: Strbuffer
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

#include <lib/strbuffer.h>
#include <memory/kmalloc.h>

strbuffer_t *strbuffer_new(uint32_t allocation)
{
	if (allocation == 0)
		allocation = 4096;

	strbuffer_t *buffer =  (strbuffer_t *)kmalloc(sizeof(strbuffer_t));
	buffer->length = 0;
	buffer->allocated = allocation;
	buffer->data = (char *)kmalloc(allocation);

	return buffer;
}

strbuffer_t *strbuffer_resize(strbuffer_t *buffer, uint32_t new_allocation)
{
	if (new_allocation <= buffer->length)
		return buffer;

	char *new_data = (char *)kmalloc(new_allocation);
	memset(new_data, 0, new_allocation);
	memcpy(new_data, buffer->data, buffer->length);
	buffer->allocated = new_allocation;
	kfree(buffer->data);
	buffer->data = new_data;

	return buffer;
}

strbuffer_t *strbuffer_append(strbuffer_t *buffer, char c)
{
	if (buffer->allocated <= buffer->length)
		strbuffer_resize(buffer, buffer->allocated * 2);

	buffer->length++;
	buffer->data[buffer->length - 1] = c;

	return buffer;
}

/*
strbuffer_t *strbuffer_prepend(strbuffer_t *buffer, char c)
{
	if (buffer->allocated <= buffer->length)
		strbuffer_resize(buffer, buffer->allocated * 2);

	memcpy
	buffer->data[0] = c;

	return buffer;
}
*/

strbuffer_t *strbuffer_concat(strbuffer_t *buffer1, strbuffer_t *buffer2)
{
	strbuffer_t *new_strbuffer = strbuffer_new(buffer1->allocated + buffer2->allocated);
	new_strbuffer->length = buffer1->length + buffer2->length;
	memcpy(new_strbuffer->data, buffer1->data, buffer1->length);
	memcpy(new_strbuffer->data + buffer1->length, buffer2->data, buffer2->length);

	return new_strbuffer;
}

char strbuffer_chr(strbuffer_t *buffer, uint32_t offset)
{
	if (offset >= buffer->length)
		return 0;

	return buffer->data[offset];
}

char strbuffer_last(strbuffer_t *buffer)
{
	if (buffer->length == 0)
		return 0;

	return buffer->data[buffer->length - 1];
}

void strbuffer_clear(strbuffer_t *buffer)
{
	if (buffer == NULL)
		return;

	buffer->length = 0;
}
